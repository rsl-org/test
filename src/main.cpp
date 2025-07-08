#define RSLTEST_SKIP
#include <rsl/test>

#include <fstream>
#include <regex>

#include <numeric>
#include <ranges>
#include <string_view>
#include <string>

#include "reporters/junit.hpp"
#include "reporters/terminal.hpp"

namespace {
void print_usage(char const* prog) {
  std::println("Usage: {} [options]", prog);
  std::println("Options:");
  std::println("  -h, --help           Show this help message and exit");
  std::println("  --list     List tests without running them.");
  std::println("  --filter <pattern>   Only run tests whose name contains <pattern>");
  std::println("  --xml[=<file>]       Output JUnit XML; if <file> omitted, writes to stdout");
}

template <std::ranges::range R>
std::string join(R&& values, std::string_view delimiter) {
  auto fold = [&](std::string a, auto b) {
    return std::move(a) + delimiter + b;
  };

  return std::accumulate(std::next(values.begin()), values.end(), std::string(values[0]),
                         fold);
}
}  // namespace

int main(int argc, char** argv) {
  bool list_only = false;
  std::string filter;
  bool use_xml = false;
  std::optional<std::string> xml_file;
  bool gtest_mode = false;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "--list" ) {
      list_only = true;
    } else if (arg == "--filter" || arg.starts_with("--gtest_filter")) {
      if (arg.starts_with("--gtest_filter")) { 
        gtest_mode = true;
        arg.remove_prefix(14);
        if (!arg.empty() && arg[0] == '=') {
          arg.remove_prefix(1);
          filter = arg;
          continue;
        }
      }

      if (i + 1 < argc) {
        filter = argv[++i];
      } else {
        std::print("Error: --filter requires an argument\n");
        return 1;
      }
    } else if (arg.rfind("--xml", 0) == 0 ) {
      use_xml = true;
      if (arg.size() > 5 && arg[5] == '=') {
        xml_file = arg.substr(6);
      }
    } else if (arg.rfind("--rsl_output=xml", 0) == 0 ) {
      use_xml = true;
      if (arg.size() > 18 && arg[18] == ':') {
        xml_file = arg.substr(19);
      }
    } else {
      std::print("Unknown option: {}\n", arg);
      print_usage(argv[0]);
      return 1;
    }
  }

  // auto root = rsl::get_tests();
  

  auto all_tests = rsl::testing::registry();
  std::vector<rsl::testing::Test> tests{};
  for (auto test_def : all_tests) {
    auto test = test_def();
    if (!filter.empty()) {
      auto full_name = join(test.full_name, gtest_mode? "." : "::");

      if (!std::regex_search(full_name, std::regex{filter})) {
        continue;
      }
    }
    tests.push_back(test);
  }

  if (list_only) {
    for (auto const& test : tests) {
      auto ns = std::span(test.full_name.begin(), test.full_name.end() - 1);
      std::println("{}.", join(ns, "_"));
      std::println("  {}", test.name);
    }
    return 0;
  }

  bool result = false;

  if (use_xml) {
    if (xml_file) {
      auto file_stream = std::ofstream(*xml_file);
      auto reporter    = rsl::testing::_testing_impl::JUnitXmlReporter(file_stream);
      result           = rsl::testing::run(tests, reporter);
    } else {
      auto reporter = rsl::testing::_testing_impl::JUnitXmlReporter(std::cout);
      result        = rsl::testing::run(tests, reporter);
    }
  } else {
    auto reporter = rsl::testing::_testing_impl::ConsoleReporter();
    result        = rsl::testing::run(tests, reporter);
  }

  return result ? 0 : 1;
}