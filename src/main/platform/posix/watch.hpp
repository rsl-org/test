#include <sys/inotify.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

constexpr std::uint32_t watch_mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM |
                                     IN_MOVED_TO | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;

struct Inotify {
  int fd                  = -1;
  Inotify(Inotify const&) = delete;
  Inotify(Inotify&&)      = default;

  Inotify& operator=(Inotify const&) = delete;
  Inotify& operator=(Inotify&&)      = delete;

  Inotify() : fd(::inotify_init1(IN_NONBLOCK)) {
    if (fd < 0) {
      throw std::runtime_error(std::format("inotify_init1: {}", std::strerror(errno)));
    }
  }

  ~Inotify() {
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

  void rm_watch(int wd) {
    inotify_rm_watch(fd, wd);
  }
};

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

enum FileEvent {
  ACCESS        = 0x00000001,                          /* File was accessed.  */
  MODIFY        = 0x00000002,                          /* File was modified.  */
  ATTRIB        = 0x00000004,                          /* Metadata changed.  */
  CLOSE_WRITE   = 0x00000008,                          /* Writtable file was closed.  */
  CLOSE_NOWRITE = 0x00000010,                          /* Unwrittable file closed.  */
  CLOSE         = (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE), /* Close.  */
  OPEN          = 0x00000020,                          /* File was opened.  */
  MOVED_FROM    = 0x00000040,                          /* File was moved from X.  */
  MOVED_TO      = 0x00000080,                          /* File was moved to Y.  */
  MOVE          = (IN_MOVED_FROM | IN_MOVED_TO),       /* Moves.  */
  CREATE        = 0x00000100,                          /* Subfile was created.  */
  DELETE        = 0x00000200,                          /* Subfile was deleted.  */
  DELETE_SELF   = 0x00000400,                          /* Self was deleted.  */
  MOVE_SELF     = 0x00000800,                          /* Self was moved.  */

  ISDIR = 0x40000000,

  /* Events sent by the kernel.  */
  UNMOUNT    = 0x00002000, /* Backing fs was unmounted.  */
  Q_OVERFLOW = 0x00004000, /* Event queued overflowed.  */
  IGNORED    = 0x00008000, /* File was ignored.  */
};

struct Watcher {
  std::unordered_map<int, std::filesystem::path> watchers;
  Inotify inotify;

  void add_directory(std::filesystem::path const& dir, bool recurse = true) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
      // throw std::runtime_error(std::format("not a directory: {}", dir.string()));
      return;
    }

    int top_wd = inotify.add_watch(dir);
    watchers.emplace(top_wd, dir);
    if (not recurse) {
      return;
    }

    for (auto const& ent : std::filesystem::recursive_directory_iterator(dir)) {
      if (ent.is_directory()) {
        try {
          int wd = inotify.add_watch(ent.path());
          watchers.emplace(wd, ent.path());
        } catch (const std::exception& ex) {
          // Non-fatal: skip directories we can't watch (permission, etc.)
          std::println("warning: cannot watch {}: {}", ent.path().string(), ex.what());
        }
      }
    }
  }

  void watch(auto&& fnc) {
    std::vector<char> pending;
    pending.reserve(8192);
    std::array<char, 8192> tmpbuf;

    while (true) {
      ssize_t n = ::read(inotify.fd, tmpbuf.data(), static_cast<int>(tmpbuf.size()));
      if (n < 0) {
        if (errno == EAGAIN) {
          ::usleep(100000);
          continue;
        }
        std::println("read error: {}", std::strerror(errno));
        break;
      }
      if (n == 0) {
        continue;
      }
      pending.insert(pending.end(), tmpbuf.data(), tmpbuf.data() + static_cast<size_t>(n));

      // decode complete events; any partial event bytes remain in pending
      auto events = try_decode_events(pending);

      for (auto const& ev : events) {
        auto it       = watchers.find(ev.wd);
        std::filesystem::path dir  = (it != watchers.end()) ? it->second : std::filesystem::path{};
        std::filesystem::path full = ev.name.empty() ? dir : dir / ev.name;
        fnc(full, FileEvent(ev.mask));

        if ((ev.mask & FileEvent::CREATE) && (ev.mask & IN_ISDIR)) {
          add_directory(full, true);
        }

        if ((ev.mask & IN_DELETE_SELF) || (ev.mask & IN_MOVE_SELF)) {
          // watched directory was deleted or moved away, remove mapping
          if (it != watchers.end()) {
            inotify.rm_watch(it->first);
            watchers.erase(it);
          }
        }
      }
    }
  }
};
