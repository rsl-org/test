#include "rsl/testing/test.hpp"
#define RSLTEST_SKIP
#include <memory>
#include <rsl/test>

#include <fstream>
#include <regex>

#include <numeric>
#include <ranges>
#include <string_view>
#include <string>

#include "reporters/junit.hpp"
#include "reporters/terminal.hpp"

#include <rsl/config>

#include "output.hpp"

namespace {
// void print_usage(char const* prog) {
//   std::println("Usage: {} [options]", prog);
//   std::println("Options:");
//   std::println("  -h, --help           Show this help message and exit");
//   std::println("  --list     List tests without running them.");
//   std::println("  --filter <pattern>   Only run tests whose name contains <pattern>");
//   std::println("  --xml[=<file>]       Output JUnit XML; if <file> omitted, writes to stdout");
// }

template <std::ranges::range R>
std::string join(R&& values, std::string_view delimiter) {
  auto fold = [&](std::string a, auto b) { return std::move(a) + delimiter + b; };

  return std::accumulate(std::next(values.begin()), values.end(), std::string(values[0]), fold);
}
}  // namespace

class TestConfig : public rsl::cli {
  rsl::testing::TestRoot tree;
  std::vector<std::string> sections;
  std::unique_ptr<rsl::testing::Output> _output;
  
public:
  [[=positional]] std::string filter    = "";
  [[=option]] std::string reporter = "colorized";
  [[=option]] bool durations            = true;
  [[=option, =flag]] bool list_tests = false;

  [[=option]] void section(std::string part) { sections.emplace_back(std::move(part)); }

  [[=option]] void output(std::string filename) {
    _output = std::make_unique<rsl::testing::FileOutput>(filename);
  }

  [[=option]] void verbosity(std::string level) {}

  explicit TestConfig() 
  : tree(rsl::testing::get_tests()) 
  , _output(new rsl::testing::ConsoleOutput())
  {}

  void apply_filter(){
    if (filter.empty()) {
      // also check other catch-alls 
      return;
    }

    rsl::testing::TestRoot new_tree;
    // rebuild the test tree with filters applied
    for (auto&& test : tree) {
      auto full_name = join(test.full_name, "::");

      if (!std::regex_search(full_name, std::regex{filter})) {
        continue;
      }
      new_tree.insert(test);
    }
    tree = new_tree;
  }

  static void print_tests(rsl::testing::TestNamespace const& current, std::size_t indent = 0) {
    auto current_indent = std::string(indent*2, ' ');
    for (auto const& ns : current.children) {
      std::println("{}{}", current_indent, ns.name);
      print_tests(ns, indent + 1);
    }

    for (auto const& test: current.tests) {
      std::println("{} - {}", current_indent, test.name);
      for (auto const& run : test.get_tests()) {
        std::println("{} - {}", std::string((indent+1)*2, ' '), run.name);
      }
    }
  }

  void run(){
    std::unique_ptr<rsl::testing::Reporter> selected_reporter;
    if (reporter == "xml") {
      selected_reporter = std::make_unique<rsl::testing::_impl::JUnitXmlReporter>(_output.get());
    } else {
      selected_reporter = std::make_unique<rsl::testing::_impl::ConsoleReporter>();
    }

    if (list_tests) {
      // tree.print(selected_reporter.get()); // TODO
      selected_reporter->list_tests(tree);
    } else {
      tree.run(selected_reporter.get());
    }
  }
};

int main(int argc, char** argv) {
  auto config = TestConfig();
  config.parse_args(argc, argv);
  config.apply_filter();
  config.run();
}