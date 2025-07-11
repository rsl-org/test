#include <algorithm>
#include <print>

#include <libassert/assert.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/output.hpp>

template <bool Colorize>
void failure_handler(libassert::assertion_info const& info) {
  // libassert::enable_virtual_terminal_processing_if_needed();  // for terminal colors on windows
  auto width          = libassert::terminal_width(libassert::stderr_fileno);
  const auto& scheme  = Colorize ? libassert::get_color_scheme() : libassert::color_scheme::blank;
  std::string message = std::string(info.action()) + " at " + info.location() + ":";
  if (info.message) {
    message += " " + *info.message;
  }
  message += "\n";
  message += info.statement(scheme) + info.print_binary_diagnostics(width, scheme) +
             info.print_extra_diagnostics(width, scheme);
  throw rsl::testing::assertion_failure(message);
}

namespace rsl::testing {
namespace _testing_impl {
std::set<TestDef>& registry();
}
TestResult Test::TestRun::run() const {
  auto ret = TestResult{.name = name};
  try {
    auto t0 = std::chrono::steady_clock::now();
    fnc();
    auto t1 = std::chrono::steady_clock::now();

    ret.passed      = !test->expect_failure;
    ret.duration_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return ret;
  } catch (assertion_failure const& failure) {
    ret.error = failure.what();
  } catch (std::exception const& exc) {
    ret.error = exc.what();
  }  //
  catch (...) {}

  ret.passed = test->expect_failure;
  return ret;
}

TestNamespace::iterator::iterator(TestNamespace const& ns) {
  flatten(ns);
  current = elements.front();
  elements.pop_front();
}

void TestNamespace::iterator::flatten(TestNamespace const& current) {
  for (auto const& ns : current.children) {
    flatten(ns);
  }
  if (!current.tests.empty()) {
    elements.push_back({current.tests.begin(), current.tests.end()});
  }
}

TestNamespace::iterator& TestNamespace::iterator::operator++() {
  if (current.it == current.end) {
    if (elements.empty()) {
      current = {};
      return *this;
    }

    current = elements.front();
    elements.pop_front();
  } else {
    ++current.it;
    if (current.it == current.end) {
      return ++*this;
    }
  }
  return *this;
}

bool TestNamespace::iterator::operator==(iterator const& other) const {
  if (current.it != other.current.it || current.end != other.current.end) {
    return false;
  }
  return elements == other.elements;
}

void TestNamespace::insert(Test const& test, std::size_t i) {
  if (i == test.full_name.size() - 1) {
    tests.push_back(test);
    return;
  }

  auto it = std::ranges::find_if(children, [&](const TestNamespace& ns) {
    return ns.name == test.full_name[i];
  });

  if (it == children.end()) {
    children.emplace_back(test.full_name[i]);
    it = std::prev(children.end());
  }

  it->insert(test, i + 1);
}

std::vector<Test::TestRun> Test::get_tests() const {
  std::vector<TestRun> tests{};
  for (auto fnc : run_fncs) {
    tests.append_range((this->*fnc)());
  }
  return tests;
}

std::size_t TestNamespace::count() const {
  std::size_t total = tests.size();
  for (auto const& ns : children) {
    total += ns.count();
  }
  return total;
}

bool TestNamespace::run(Reporter* reporter) {
  reporter->enter_namespace(name);
  bool status = true;
  for (auto& ns : children) {
    status &= ns.run(reporter);
  }

  for (auto& test : tests) {
    reporter->before_test(test);
    for (auto const& test_run : test.get_tests()) {
      auto result = test_run.run();
      reporter->after_test(result);
      // TODO don't discard test result
    }
  }
  reporter->exit_namespace(name);
  return status;
}

bool TestRoot::run(Reporter* reporter) {
  libassert::set_failure_handler(failure_handler<true>);
  reporter->before_run(*this);
  bool status = TestNamespace::run(reporter);
  libassert::set_failure_handler(libassert::default_failure_handler);
  // TODO after_run
  return status;
}

TestRoot get_tests() {
  TestRoot root;
  for (auto test_def : rsl::testing::_testing_impl::registry()) {
    auto test = test_def();
    root.insert(test);
  }
  return root;
}

static void print_tests(rsl::testing::TestNamespace const& current, std::size_t indent = 0) {
  auto current_indent = std::string(indent * 2, ' ');
  for (auto const& ns : current.children) {
    std::println("{}{}", current_indent, ns.name);
    print_tests(ns, indent + 1);
  }

  for (auto const& test : current.tests) {
    std::println("{}- {}", current_indent, test.name);
    for (auto const& run : test.get_tests()) {
      std::println("{}- {}", std::string((indent + 1) * 2, ' '), run.name);
    }
  }
}

void Reporter::list_tests(TestNamespace const& tests) {
  print_tests(tests);
}

}  // namespace rsl::testing