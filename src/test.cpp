#include <algorithm>
#include <ranges>
#include <print>
#include <chrono>

#include <rsl/testing/assert.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/output.hpp>
#include <rsl/testing/util.hpp>
#include <rsl/testing/_testing_impl/discovery.hpp>

#include "capture.hpp"
#include "rsl/testing/test_case.hpp"

#include <cpptrace/basic.hpp>
#include <cpptrace/utils.hpp>

void cleanup_frames(cpptrace::stacktrace& trace, std::string_view test_name) {
  std::vector<cpptrace::stacktrace_frame> frames;
  for (auto const& frame : trace.frames | std::views::drop(1)) {
    frames.push_back(frame);
    if (cpptrace::prune_symbol(frame.symbol) == test_name) {
      break;
    }
  }
  trace.frames = frames;
}

void failure_handler(libassert::assertion_info const& info) {
  // libassert::enable_virtual_terminal_processing_if_needed();  // for terminal colors on windows
  constexpr bool Colorize = false;
  auto width              = libassert::terminal_width(libassert::stderr_fileno);
  const auto& scheme  = Colorize ? libassert::get_color_scheme() : libassert::color_scheme::blank;
  std::string message = std::string(info.action()) + " at " + info.location() + ":";
  if (info.message) {
    message += " " + *info.message;
  }
  message += "\n";
  message +=
      info.statement(scheme) + info.print_binary_diagnostics(width, scheme) +
      info.print_extra_diagnostics(width, scheme);  // + info.print_stacktrace(width, scheme);

  auto trace = info.get_stacktrace();
  cleanup_frames(trace, rsl::testing::_testing_impl::assertion_counter().test_name);
  message += trace.to_string(Colorize);
  throw rsl::testing::assertion_failure(message);
}

namespace rsl::testing {
namespace _testing_impl {
std::set<TestDef>& registry() {
  static std::set<TestDef> data;
  return data;
}

AssertionTracker& assertion_counter() {
  static AssertionTracker counter{};
  return counter;
}
}  // namespace _testing_impl

bool TestRoot::run(Reporter* reporter) {
  libassert::set_failure_handler(failure_handler);
  std::println("failure handler set");
  reporter->before_run(*this);
  bool status = TestNamespace::run(reporter);
  libassert::set_failure_handler(libassert::default_failure_handler);
  // TODO after_run
  reporter->after_run({});
  return status;
}

bool TestNamespace::run(Reporter* reporter) {
  if (!name.empty()) {
    reporter->enter_namespace(name);
  }
  bool status = true;
  for (auto& ns : children) {
    status &= ns.run(reporter);
  }

  for (auto& test : tests) {
    auto runs = test.get_tests();
    reporter->before_test_group(test);
    std::vector<TestResult> results;
    if (!test.skip()) {
      std::vector<TestResult> results;
      for (auto const& test_run : test.get_tests()) {
        auto& tracker      = _testing_impl::assertion_counter();
        tracker.assertions = {};
        tracker.test_name  = join_str(test.full_name, "::");

        reporter->before_test(test_run);
        auto result = test_run.run();
        reporter->after_test(result);

        result.assertions = tracker.assertions;
        results.push_back(result);
      }
    } else {
      reporter->before_test(TestCase{&test, +[]{}, std::string(test.name)});
      
      // TODO stringify skipped tests properly
      auto result = TestResult{&test, std::string(test.name) + "(...)", TestOutcome::PASS};
      reporter->after_test(result);
      results.push_back(result);
    }

    reporter->after_test_group(results);
  }
  if (!name.empty()) {
    reporter->exit_namespace(name);
  }
  return status;
}

TestResult TestCase::run() const {
  auto ret = TestResult{.test = test, .name = name};
  try {
    Capture _out(stdout, ret.stdout);
    Capture _err(stderr, ret.stderr);

    auto t0 = std::chrono::steady_clock::now();
    fnc();
    auto t1 = std::chrono::steady_clock::now();

    ret.outcome      = TestOutcome(!test->expect_failure);
    ret.duration_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return ret;
  } catch (assertion_failure const& failure) {
    ret.failure += failure.what();
  } catch (std::exception const& exc) {  //
    ret.exception += exc.what();
  } catch (std::string const& msg) {  //
    ret.failure += msg;
  } catch (std::string_view msg) {  //
    ret.failure += msg;
  } catch (char const* msg) {  //
    ret.failure += msg;
  } catch (...) { ret.exception += "unknown exception thrown"; }

  ret.outcome = TestOutcome(test->expect_failure);
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

std::size_t TestNamespace::count() const {
  std::size_t total = tests.size();
  for (auto const& ns : children) {
    total += ns.count();
  }
  return total;
}

void TestNamespace::filter(std::span<std::string const> parts) {
  if (parts.empty()) {
    return;
  }

  std::string_view current          = parts.front();
  std::span<std::string const> next = parts.subspan(1);

  auto it = std::ranges::find_if(children, [&](TestNamespace& ns) { return ns.name == current; });

  if (it != children.end()) {
    tests.clear();
    it->filter(next);
    if (it->children.empty() && it->tests.empty()) {
      children.clear();
    } else {
      children = {*it};
    }
    return;
  } else {
    std::erase_if(tests, [&](const Test& t) { return t.name != current; });
    children.clear();
  }
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