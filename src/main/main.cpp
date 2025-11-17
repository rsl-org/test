#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <dlfcn.h>

#include "compile_pool.hpp"
#include "config_parser.hpp"
#include <rsl/testing/_testing_impl/discovery.hpp>

#include "platform/library.hpp"

#include "platform/posix/watch.hpp"
#include "rsl/testing/output.hpp"

struct TestSet {
  void* handle;
  std::set<rsl::testing::TestDef> tests;
};

int main() {
  using namespace rsl::testing::_impl_main;
  constexpr bool incremental = false;

  const auto executable_path = std::filesystem::canonical("/proc/self/exe").parent_path();
  const std::filesystem::path config_path = executable_path / "test-runner.json";

  ConfigParser runner(config_path);
  auto test_tus = runner.expand();  // TODO split config and test discovery

  CompilePool pool{};
  for (auto&& tu : test_tus) {
    pool.submit(tu);
  }

  std::unordered_map<std::filesystem::path, TestSet> test_sets;

  for (auto&& [tu, result] : pool.collect()) {
    if (result.exit_code != 0) {
      std::println("ERROR!");
      continue;
    }

    if (not rsl::testing::_testing_impl::registry().empty()) {
      std::println("test registry not empty");
      rsl::testing::_testing_impl::registry().clear();
    }
    // check if we have the key already, make sure old one is unloaded

    void* handle = load_library(tu.out_path.string());
    if (handle != nullptr) {
      test_sets[tu.out_path] = {handle, rsl::testing::_testing_impl::registry()};
    }
    rsl::testing::_testing_impl::registry().clear();
  }

  if (not rsl::testing::_testing_impl::registry().empty()) {
    std::println("test registry not empty");
  }

  rsl::testing::TestRoot root;
  for (auto&& [path, test_set] : test_sets) {
    std::println("{} -> {}", path.string(), test_set.tests.size());
    for (auto test : test_set.tests) {
      root.insert(test());
    }
  }

  std::unique_ptr<rsl::testing::Reporter> selected_reporter;
  selected_reporter = rsl::testing::Reporter::make("plain");
  root.run(selected_reporter.get());

  for (auto& [path, test_set] : test_sets) {
    dlclose(test_set.handle);
    test_set.tests = {};
  }

  if (incremental) {
    Watcher watch{};
    const auto test_paths = runner.config_.at("project").at("test_path");
    for (auto path : test_paths) {
      watch.add_directory(path);
    }
    watch.watch([&](auto path, FileEvent event) {
      if ((event & FileEvent::MODIFY) == FileEvent::MODIFY) {
        // compile TU

        if (auto it = test_sets.find(path); it != test_sets.end()) {
          TestSet& set = it->second;
          
          // clean up old test
          if (set.handle != nullptr) {
            dlclose(set.handle);
            set.handle = nullptr;
          }
          set.tests = {};
        }

        // load TU
      }
    });
  }
}