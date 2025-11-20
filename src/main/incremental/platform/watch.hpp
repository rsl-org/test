#pragma once

#include <filesystem>
#include <span>
#include <unordered_map>
#include <vector>
namespace rsl::testing::_impl_main {

struct WatcherImpl;
class Watcher {
  WatcherImpl* impl;
  std::unordered_map<int, std::filesystem::path> watchers; // TODO flip
  std::vector<char> pending;

public:
  Watcher();
  ~Watcher() noexcept;

  [[nodiscard]] uintptr_t get_handle() const;
  void on_readable(std::span<char const> data);
  void add_watch(std::filesystem::path const& dir, bool recurse = true);
  void rm_watch(std::filesystem::path const& dir);
};
}  // namespace rsl::testing::_impl_main