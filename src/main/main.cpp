#include <memory>
#include <ranges>
#include <string_view>
#include <string>

#include <rsl/config>
#include <rsl/testing/output.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/util.hpp>
#include <rsl/testing/_testing_impl/factory.hpp>
#include "output.hpp"

std::string_view base_name(std::string_view name) {
  auto lt    = name.find('<');
  auto paren = name.find('(');
  auto pos   = std::min(lt, paren);
  return pos == std::string_view::npos ? name : name.substr(0, pos);
}

std::vector<std::string_view> split_filter_path(std::string_view filter) {
  if (filter.starts_with("::")) {
    filter.remove_prefix(2);
  }

  std::vector<std::string_view> parts;

  while (true) {
    auto next  = filter.find("::");
    auto templ = filter.find('<');

    if (templ != std::string_view::npos && (next == std::string_view::npos || templ < next)) {
      parts.push_back(filter);
      break;
    }

    if (next == std::string_view::npos) {
      parts.push_back(filter);
      break;
    }

    parts.push_back(filter.substr(0, next));
    filter.remove_prefix(next + 2);
  }

  return parts;
}

void filter_test_tree(rsl::testing::TestRoot& root,
                      std::string_view filter,
                      std::vector<std::string> subfilters) {
  if (filter.empty() || filter == "[.],*") {
    return;
  }

  auto parts = split_filter_path(filter);

  for (auto part : std::ranges::reverse_view(parts)) {
    subfilters.insert(subfilters.begin(), std::string(part));
  }
  if (!subfilters.empty()) {
    // cut off template arguments and parameters
    subfilters.back() = base_name(subfilters.back());
  }
  root.filter(subfilters);
}

class[[= rsl::cli::description("rsl::test (in Catch2 v3.8.1 compatibility mode)")]] TestConfig
    : public rsl::cli {
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
    filter_test_tree(tree, filter, sections);
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