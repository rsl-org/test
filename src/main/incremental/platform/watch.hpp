#pragma once

#include <chrono>
#include <filesystem>
#include <span>
#include <unordered_map>
#include <vector>
#include "../incremental.hpp"

namespace rsl::testing::_impl_main {
struct WatcherImpl;

struct HeaderDependencies {
  std::filesystem::path file;
  std::vector<std::filesystem::path> dependencies;
};

inline std::string_view trim(std::string_view data) {
  auto first = data.find_first_not_of(" \t");
  auto last  = data.find_last_not_of(" \t");
  if (first == last) {
    return {};
  }
  return data.substr(first, last - first + 1);
}

inline std::vector<std::filesystem::path> parse_dependencies(std::string_view input) {
  auto colon = input.find(':');
  if (colon == std::string_view::npos) {
    return {};
  }
  input.remove_prefix(colon + 1);

  std::vector<std::filesystem::path> deps;

  for (auto line : input | std::views::split('\n')) {
    auto line_sv = std::string_view(&*line.begin(), std::ranges::distance(line));
    if (!line_sv.empty() && line_sv.back() == '\\') {
      line_sv.remove_suffix(1);
    }
    line_sv = trim(line_sv);
    if (!line_sv.empty()) {
      deps.emplace_back(line_sv);
    }
  }

  return deps;
}

inline bool is_relative_to(std::filesystem::path const& path, std::filesystem::path const& base) {
  auto abs_path           = std::filesystem::weakly_canonical(path);
  auto abs_base           = std::filesystem::weakly_canonical(base);
  auto [it_path, it_base] = std::ranges::mismatch(abs_path, abs_base);
  return it_base == abs_base.end();
}



class Watcher {
  WatcherImpl* impl;
  std::unordered_map<std::filesystem::path, int> watchers;

  std::vector<char> pending;
  IncrementalRunner* runner;

  std::set<std::filesystem::path> tests;
  std::unordered_map<std::filesystem::path, std::set<std::filesystem::path const*>> dependencies;

public:
  Watcher() = delete;
  explicit Watcher(IncrementalRunner& runner);
  ~Watcher() noexcept;

  [[nodiscard]] uintptr_t get_handle() const;
  void on_readable(std::span<char const> data);
  void add_watch(std::filesystem::path const& dir, bool recurse = true);
  void rm_watch(std::filesystem::path const& dir);

  void update_dependencies(std::filesystem::path const& path = {}) {
    std::vector<std::filesystem::path> paths{};
    if (path.empty()) {
      paths = runner->discover_tests();
    } else {
      paths = {path};
    }
    
    for (auto&& tu : runner->expand_tests(paths, true)) {
      runner->pool.submit(tu);
    }
    runner->pool.wait();
    
    std::set<std::filesystem::path> folders;
    for (auto [r, result] : runner->pool.collect()) {
      if (result.exit_code != 0) {
        continue;
      }

      // TODO remove everything referring to r.source_path first
      auto [it, _] = tests.insert(r.source_path);

      for (auto dependency : parse_dependencies(result.stdout_str)) {
        dependency = std::filesystem::canonical(dependency);
        if (dependency == r.source_path) { continue; }
        if (not is_relative_to(dependency, runner->config.project.project_path)) {
          continue;
        }
        dependencies[dependency].insert(&*it);
        folders.insert(dependency.parent_path());
      }
    }
    
    for (auto const& folder : folders) {
      add_watch(folder);
    }
  }

  void file_modified(std::filesystem::path const& path) {
    auto canonical = std::filesystem::canonical(path);
    if (tests.contains(canonical)) {
      std::println("test modified: {}", canonical.string());
      update_dependencies(canonical);
      runner->recompile(runner->expand_tests({path}));
    } else {
      std::println("test dependency modified: {} {}", canonical.string(), dependencies[canonical].size());
      std::vector<std::filesystem::path> affected;
      for (auto* it : dependencies[canonical]) {
        affected.push_back(*it);
      }
      runner->recompile(runner->expand_tests(affected));
    }
  }

  void file_deleted(std::filesystem::path const& path) {}
};
}  // namespace rsl::testing::_impl_main