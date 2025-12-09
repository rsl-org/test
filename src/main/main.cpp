#include <filesystem>
#include <rsl/config>

#include <rsl/testing/_testing_impl/discovery.hpp>
#include <rsl/testing/output.hpp>

#include "incremental.hpp"
#include "platform/library.hpp"
#include "platform/stdin.hpp"
#include "platform/event_loop.hpp"

#include "output.hpp"

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

void sigbus_handler(int sig) {
  void* bt[20];
  int n = backtrace(bt, 20);
  backtrace_symbols_fd(bt, n, STDERR_FILENO);
  _exit(1);
}

class[[= rsl::cli::description("rsl::test (in Catch2 v3.8.1 compatibility mode)")]] CLI
    : public rsl::cli {
  rsl::testing::TestRoot tree;
  std::vector<std::string> sections;
  std::unique_ptr<rsl::testing::Output> _output;

public:
  [[= positional]] std::string filter     = "";
  [[= option]] std::string reporter       = "plain";
  [[= option]] bool durations             = true;
  [[= option]] bool use_colour            = true;
  [[ = option, = flag ]] bool list_tests  = false;
  [[ = option, = flag ]] bool interactive = false;

  [[ = option, = shorthand("c") ]] void section(std::string part) {
    sections.emplace_back(std::move(part));
  }

  [[= option]] void output(std::string filename) {
    _output = std::make_unique<rsl::testing::FileOutput>(filename);
  }

  [[= option]] void verbosity(std::string level) {}

  explicit CLI() : tree(rsl::testing::get_tests()), _output(new rsl::testing::ConsoleOutput()) {}

  void apply_filter() {}

  static void print_tests(rsl::testing::TestNamespace const& current, std::size_t indent = 0) {
    auto current_indent = std::string(indent * 2, ' ');
    for (auto const& ns : current.children) {
      std::println("{}{}", current_indent, ns.name);
      print_tests(ns, indent + 1);
    }

    for (auto const& test : current.tests) {
      std::println("{} - {}", current_indent, test.name);
      for (auto const& run : test.get_tests()) {
        std::println("{} - {}", std::string((indent + 1) * 2, ' '), run.name);
      }
    }
  }
};

int main(int argc, char** argv) {
  signal(SIGBUS, sigbus_handler);

  using namespace rsl::testing::_impl_main;
  const auto executable_path = std::filesystem::canonical("/proc/self/exe").parent_path();
  const std::filesystem::path config_path = executable_path / "test-runner.json";

  auto runner = IncrementalRunner(config_path);

  auto args = CLI();
  args.parse_args(argc, argv);

  if (args.interactive) {
    auto watch    = Watcher(runner);
    auto commands = TerminalCommand(runner, watch);
    auto loop     = EventLoop(commands, watch);
    loop.run();
  }

  // if (watch_dependencies) {
  //   watch.update_dependencies();
  // }
}
