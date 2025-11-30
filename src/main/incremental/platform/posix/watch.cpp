#include "../watch.hpp"

#include <sys/stat.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <print>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include "incremental/incremental.hpp"

#include <nlohmann/json.hpp>

namespace {
constexpr std::uint32_t watch_mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM |
                                     IN_MOVED_TO | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;

struct InotifyEvent {
  int wd;
  std::uint32_t mask;
  std::uint32_t cookie;
  std::string name;  // empty when event->len == 0
};

// Decode as many complete events as available in 'pending'.
// Leaves any trailing partial bytes in `pending`
static std::vector<InotifyEvent> try_decode_events(std::vector<char>& pending) {
  std::vector<InotifyEvent> out;
  constexpr std::size_t header_size = sizeof(inotify_event::wd) + sizeof(inotify_event::mask) +
                                      sizeof(inotify_event::cookie) + sizeof(inotify_event::len);

  std::size_t offset = 0;
  while (true) {
    if (pending.size() - offset < header_size) {
      // not enough data, try again
      break;
    }

    InotifyEvent event{};

    std::memcpy(&event.wd, pending.data() + offset, sizeof(inotify_event::wd));
    offset += sizeof(inotify_event::wd);

    std::memcpy(&event.mask, pending.data() + offset, sizeof(inotify_event::mask));
    offset += sizeof(inotify_event::mask);

    std::memcpy(&event.cookie, pending.data() + offset, sizeof(inotify_event::cookie));
    offset += sizeof(inotify_event::cookie);

    uint32_t len = 0;
    std::memcpy(&len, pending.data() + offset, sizeof(inotify_event::len));
    offset += sizeof(inotify_event::len);

    // Total bytes this event occupies
    if (pending.size() - offset < len) {
      // not enough data, try again
      offset -= header_size;
      break;
    }

    std::string name{};
    if (len > 0) {
      name.assign(pending.data() + offset, pending.data() + offset + len);
      if (!name.empty() && name.back() == '\0') {
        // may include sentinel, strip it
        name.pop_back();
      }
    }

    out.push_back({event.wd, event.mask, event.cookie, name});
    offset += len;
  }

  // Erase consumed bytes from the front of pending.
  if (offset > 0) {
    pending.erase(pending.begin(), pending.begin() + offset);
  }

  return out;
}

struct stat path_stat(std::filesystem::path const& path) {
  struct stat out{};
  if (auto ret = ::stat(path.c_str(), &out); ret < 0) {
    throw std::runtime_error(std::format("stat: {}", std::strerror(errno)));
  }
  return out;
}
}  // namespace

namespace rsl::testing::_impl_main {
struct WatcherImpl {
  int fd = -1;
  std::unordered_map<std::filesystem::path, time_t> last_modified;

  WatcherImpl(WatcherImpl const&) = delete;
  WatcherImpl(WatcherImpl&&)      = default;

  WatcherImpl& operator=(WatcherImpl const&) = delete;
  WatcherImpl& operator=(WatcherImpl&&)      = delete;

  WatcherImpl() : fd(::inotify_init1(IN_NONBLOCK)) {
    if (fd < 0) {
      throw std::runtime_error(std::format("inotify_init1: {}", std::strerror(errno)));
    }
  }

  ~WatcherImpl() {
    if (fd >= 0) {
      ::close(fd);
    }
  }

  [[nodiscard]] int add_watch(const std::filesystem::path& p) const {
    int wd = ::inotify_add_watch(fd, p.c_str(), watch_mask);
    if (wd < 0) {
      throw std::runtime_error(
          std::format("inotify_add_watch({}): {}", p.string(), std::strerror(errno)));
    }
    return wd;
  }

  void rm_watch(int wd) { inotify_rm_watch(fd, wd); }
};

Watcher::Watcher(IncrementalRunner& runner) : impl(new WatcherImpl()), runner(&runner) {}
Watcher::~Watcher() noexcept {
  delete impl;
}
uintptr_t Watcher::get_handle() const {
  return impl->fd;
}

void Watcher::on_readable(std::span<char const> data) {
  constexpr auto debounce_threshold = std::chrono::milliseconds(500);

  pending.append_range(data);
  auto events = try_decode_events(pending);
  for (auto const& ev : events) {
    auto it = std::ranges::find_if(watchers, [&](auto&& obj) { return obj.second == ev.wd; });
    std::filesystem::path dir  = (it != watchers.end()) ? weakly_canonical(it->first) : std::filesystem::path{};
    std::filesystem::path full = ev.name.empty() ? dir : dir / ev.name;

    if ((ev.mask & IN_CREATE) && (ev.mask & IN_ISDIR)) {
      add_watch(full, true);
    }

    if ((ev.mask & IN_DELETE_SELF) || (ev.mask & IN_MOVE_SELF)) {
      // watched directory was deleted or moved away, remove mapping
      if (it != watchers.end()) {
        impl->rm_watch(it->second);
        watchers.erase(it);
      }
      file_deleted(full);
    }

    if ((ev.mask & IN_MODIFY)) {
      auto info = path_stat(full);
      if (impl->last_modified[full] >= info.st_mtime) {
        continue;
      }
      impl->last_modified[full] = info.st_mtime;
      file_modified(full);
    }
  }
}

void Watcher::add_watch(std::filesystem::path const& dir, bool recurse) {
  if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
    // throw std::runtime_error(std::format("not a directory: {}", dir.string()));
    return;
  }

  if (watchers.contains(dir)) {
    // already watching
    return;
  }

  // std::println("watching {} for changes", dir.string());
  std::println("{}", nlohmann::json({{"action", "add_watch"}, {"path", dir.string()}}).dump());

  int top_wd = impl->add_watch(dir);
  watchers.emplace(dir, top_wd);
  if (not recurse) {
    return;
  }

  for (auto const& ent : std::filesystem::recursive_directory_iterator(dir)) {
    if (ent.is_directory()) {
      try {
        int wd = impl->add_watch(ent.path());
        watchers.emplace(ent.path(), wd);
      } catch (const std::exception& ex) {
        // Non-fatal: skip directories we can't watch (permission, etc.)
        std::println("warning: cannot watch {}: {}", ent.path().string(), ex.what());
      }
    }
  }
}

void Watcher::rm_watch(std::filesystem::path const& dir) {
  auto it = watchers.find(dir);
  if (it != watchers.end()) {
    impl->rm_watch(it->second);
    watchers.erase(it);
  }
}

}  // namespace rsl::testing::_impl_main
