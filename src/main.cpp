#include <fstream>
#include <cpptest.hpp>
#include <regex>

#include "reporters/junit.hpp"
#include "reporters/terminal.hpp"

namespace {
void print_usage(char const* prog) {
  std::println("Usage: {} [options]", prog);
  std::println("Options:");
  std::println("  -h, --help           Show this help message and exit");
  std::println("  --list               List all tests and exit");
  std::println("  --filter <pattern>   Only run tests whose name contains <pattern>");
  std::println("  --xml[=<file>]       Output JUnit XML; if <file> omitted, writes to stdout");
}
}  // namespace

int main(int argc, char** argv) {
  bool list_only = false;
  std::string filter;
  bool use_xml = false;
  std::optional<std::string> xml_file;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "--list") {
      list_only = true;
    } else if (arg == "--filter") {
      if (i + 1 < argc) {
        filter = argv[++i];
      } else {
        std::print("Error: --filter requires an argument\n");
        return 1;
      }
    } else if (arg.rfind("--xml", 0) == 0) {
      use_xml = true;
      if (arg.size() > 5 && arg[5] == '=') {
        xml_file = arg.substr(6);
      }
    } else {
      std::print("Unknown option: {}\n", arg);
      print_usage(argv[0]);
      return 1;
    }
  }

  auto all_tests = cpptest::registry();
  std::vector<cpptest::Test> tests     = all_tests;
  if (!filter.empty()) {
    tests = {};
    for (auto& test : all_tests) {
        if (!std::regex_search(test.name, std::regex{filter})) {
            continue;
        }
        tests.push_back(test);
    }
  }

  if (list_only) {
    for (auto& t : all_tests)
      std::println("{}", t.name);
    return 0;
  }

  bool result = false;

  if (use_xml) {
    if (xml_file) {
      auto file_stream = std::ofstream(*xml_file);
      auto reporter    = cpptest::impl::JUnitXmlReporter(file_stream);
      result           = cpptest::run(tests, reporter);
    } else {
      auto reporter = cpptest::impl::JUnitXmlReporter(std::cout);
      result        = cpptest::run(tests, reporter);
    }
  } else {
    auto reporter = cpptest::_impl::ConsoleReporter();
    result        = cpptest::run(tests, reporter);
  }

  return result ? 0 : 1;
}