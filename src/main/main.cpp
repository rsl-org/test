#include <filesystem>

#include <nlohmann/json.hpp>

#include <rsl/testing/_testing_impl/discovery.hpp>
#include <rsl/testing/output.hpp>
#include <string>
#include <unordered_map>

#include "incremental/incremental.hpp"
#include "incremental/platform/library.hpp"
#include "incremental/platform/stdin.hpp"
#include "incremental/platform/event_loop.hpp"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig) {
    void *bt[20];
    int n = backtrace(bt, 20);
    backtrace_symbols_fd(bt, n, STDERR_FILENO);
    _exit(1);
}

int main() {
  signal(SIGBUS, handler);

  using namespace rsl::testing::_impl_main;
  constexpr bool incremental        = true;
  constexpr bool watch_dependencies = true;

  const auto executable_path = std::filesystem::canonical("/proc/self/exe").parent_path();
  const std::filesystem::path config_path = executable_path / "test-runner.json";

  auto runner      = IncrementalRunner(config_path);
  auto test_inputs = runner.discover_tests();
  
  std::unique_ptr<rsl::testing::Reporter> selected_reporter;
  selected_reporter = rsl::testing::Reporter::make("plain");

  auto root = runner.recompile(test_inputs);
  runner.update_compdb();

  root.run(runner.reporter.get());

  
  if (incremental) {
    Watcher watch{runner};
    for (auto const& path : runner.config.project.test_path) {
      watch.add_watch(path);
    }

    if (watch_dependencies) {
      watch.update_dependencies();
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