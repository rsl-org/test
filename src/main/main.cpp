#include "rsl/testing/output.hpp"
#include "rsl/testing/test.hpp"
#define RSLTEST_SKIP
#include <memory>
#include <rsl/test>

#include <regex>

#include <numeric>
#include <ranges>
#include <string_view>
#include <string>

#include <rsl/config>
#include <rsl/testing/_testing_impl/factory.hpp>
#include "output.hpp"

namespace {
template <std::ranges::range R>
std::string join(R&& values, std::string_view delimiter) {
  auto fold = [&](std::string a, auto b) { return std::move(a) + delimiter + b; };

  return std::accumulate(std::next(values.begin()), values.end(), std::string(values[0]), fold);
}

}  // namespace

class[[= rsl::cli::description("rsl::test (in Catch2 v3.8.1 compatibility mode)")]] TestConfig : public rsl::cli {
  rsl::testing::TestRoot tree;
  std::vector<std::string> sections;
  std::unique_ptr<rsl::testing::Output> _output;

public:
  [[= positional]] std::string filter    = "";
  [[= option]] std::string reporter      = "plain";
  [[= option]] bool durations            = true;
  [[ = option, = flag ]] bool list_tests = false;
  [[= option]] bool use_colour           = true;

  [[ = option, = shorthand("c") ]] void section(std::string part) {
    sections.emplace_back(std::move(part));
  }

  [[= option]] void output(std::string filename) {
    _output = std::make_unique<rsl::testing::FileOutput>(filename);
  }

  [[= option]] void verbosity(std::string level) {}

  explicit TestConfig()
      : tree(rsl::testing::get_tests())
      , _output(new rsl::testing::ConsoleOutput()) {}

  void apply_filter() {
    if (filter.empty() || filter == "[.],*") {
      return;
    }
    std::string_view filter_view = filter;
    std::vector<std::string> names;
    std::size_t idx = 0;
    while ((idx = filter_view.find(',')) != filter_view.npos) {
      names.emplace_back(filter_view.substr(0, idx));
      filter_view.remove_prefix(idx + 1);
    }
    if (!filter_view.empty()) {
      names.emplace_back(filter_view);
    }

    rsl::testing::TestRoot new_tree;
    // rebuild the test tree with filters applied
    for (auto&& test : tree) {
      auto full_name = join(test.full_name, "::");

      for (auto const& name : names) {
        if (name == full_name || name == test.name || name == test.full_name[0]) {
          new_tree.insert(test);
        }
      }
      // if (!std::regex_search(full_name, std::regex{filter})) {
      //   continue;
      // }
    }
    tree = new_tree;
  }

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

  void run() {
    std::unique_ptr<rsl::testing::Reporter> selected_reporter;
    if (reporter.empty()) {
      selected_reporter = rsl::testing::Reporter::make("plain");
    } else {
      selected_reporter = rsl::testing::Reporter::make(reporter);
    }

    if (list_tests) {
      // tree.print(selected_reporter.get()); // TODO
      selected_reporter->list_tests(tree);
    } else {
      tree.run(selected_reporter.get());
    }
    selected_reporter->finalize(*_output);
  }
};

int main(int argc, char** argv) {
  auto config = TestConfig();
  config.parse_args(argc, argv);
  config.apply_filter();
  config.run();
}