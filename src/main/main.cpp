#include <filesystem>

#include <nlohmann/json.hpp>

#include <rsl/testing/_testing_impl/discovery.hpp>
#include <rsl/testing/output.hpp>

#include "incremental/incremental.hpp"
#include "incremental/platform/library.hpp"
#include "incremental/platform/stdin.hpp"
#include "incremental/platform/event_loop.hpp"

int main() {
  using namespace rsl::testing::_impl_main;
  constexpr bool incremental = true;

  const auto executable_path = std::filesystem::canonical("/proc/self/exe").parent_path();
  const std::filesystem::path config_path = executable_path / "test-runner.json";

  auto runner      = IncrementalRunner(config_path);
  auto test_inputs = runner.discover_tests();

  runner.recompile(runner.expand_tests(test_inputs));

  rsl::testing::TestRoot root;

  auto update_tree = [&](auto file_path) {
    // remove updated tests from tree
    // for (auto&& [path, _] : runner.test_sets) {
    //   root.remove_by_path(path.string());
    // }

    // rebuild root
    root = {};
    // insert
    rsl::testing::TestRoot tests;
    for (auto&& [path, test_set] : runner.test_sets) {
      // std::println("{} -> {}", path.string(), test_set.tests.size());
      for (auto test_def : test_set.tests) {
        auto test = test_def(path);
        // root.insert(test);
        // if (file_path == path) {
        tests.insert(test);
        // }
      }
    }
    return tests;
  };
  update_tree("");

  std::unique_ptr<rsl::testing::Reporter> selected_reporter;
  selected_reporter = rsl::testing::Reporter::make("plain");
  root.run(selected_reporter.get());

  for (auto& [path, test_set] : runner.test_sets) {
    unload_library(test_set.handle);
    test_set.tests = {};
  }

  if (incremental) {
    Watcher watch{runner};
    for (auto const& path : runner.config.project.test_path) {
      watch.add_watch(path);
    }

    // auto watch_fnc = [&](auto path, FileEvent event) {
    //   if ((event & FileEvent::MODIFY) == FileEvent::MODIFY) {
    //     // compile TU
    //     runner.recompile(runner.expand_tests({path}));
    //     // load TU
    //     auto updated = update_tree(path);
    //     // updated.run(selected_reporter.get(), false);
    //   }
    // };
    TerminalCommand commands{runner, watch};
    
    auto loop = EventLoop(commands, watch);
    loop.run();
  }
}