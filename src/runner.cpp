#include <string>
#include <ranges>
#include <vector>
#include <functional>
#include <chrono>
#include <print>

#include <rsl/source_location>
#include <rsl/testing/assert.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/result.hpp>
#include <rsl/testing/output.hpp>
#include <rsl/testing/util.hpp>

#include <cpptrace/basic.hpp>
#include <cpptrace/utils.hpp>

#include "capture.hpp"
#include "coverage/coverage.hpp"

namespace {
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
  throw rsl::testing::assertion_failure(
      message,
      rsl::source_location(info.file_name, info.function, info.line));
}

void print_tests(rsl::testing::TestNamespace const& current, std::size_t indent = 0) {
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
}  // namespace

namespace rsl::testing {
void Reporter::list_tests(TestNamespace const& tests) {
  print_tests(tests);
}

bool TestRoot::run(Reporter* reporter) {
  libassert::set_failure_handler(failure_handler);
  std::println("failure handler set");
  reporter->before_run(*this);
  bool status = TestNamespace::run(reporter);
  libassert::set_failure_handler(libassert::default_failure_handler);
  // TODO after_run
  reporter->after_run();
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
    std::vector<Result> results;
    if (!test.skip()) {
      for (auto const& test_run : test.get_tests()) {
        auto& tracker      = _testing_impl::assertion_counter();
        tracker.assertions = {};
        tracker.test_name  = join_str(test.full_name, "::");

        reporter->before_test(test_run);
        auto result = test_run.run();
        result.assertions = tracker.assertions;

        reporter->after_test(result);
        results.push_back(result);
      }
    } else {
      reporter->before_test(TestCase{&test, +[] {}, std::string(test.name)});

      // TODO stringify skipped tests properly
      auto result = Result{&test, std::string(test.name) + "(...)", TestOutcome::SKIP};
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

namespace {
void run_test(void const* test) {
  (*static_cast<std::function<void()> const*>(test))();
}

auto resolve_pc(std::uintptr_t pc) {
  auto raw_trace = cpptrace::raw_trace{{pc}};
  auto trace     = raw_trace.resolve();
  return trace.frames[0];
}

auto filter_coverage(rsl::coverage::CoverageReport* data, std::size_t size) {
  std::unordered_map<std::string, std::vector<LineCoverage>> coverage;

  for (std::size_t idx = 0; idx < size; ++idx) {
    auto resolved = resolve_pc(data[idx].pc);
    if (resolved.filename.empty() || (int)resolved.line.value() < 0) {
      continue;
    }
    if (resolved.filename.contains("/../include/c++/")) {
      continue;
    }
    coverage[resolved.filename].push_back({resolved.line.value(), data[idx].hits});
  }

  std::vector<FileCoverage> result;
  for (auto const& [name, cov] : coverage) {
    result.emplace_back(name, cov);
  }
  return result;
}
}  // namespace

Result TestCase::run() const {
  auto ret = Result{.test = test, .name = name};
  try {
    // Capture _out(stdout, ret.stdout);
    // Capture _err(stderr, ret.stderr);

    auto t0 = std::chrono::steady_clock::now();
    if (_rsl_test_run_with_coverage != nullptr) {
      // rsltest_cov was linked in -> run with coverage
      rsl::coverage::CoverageReport* reports = nullptr;
      std::size_t report_count               = 0;
      auto finalize = [&] {
        ret.coverage = filter_coverage(reports, report_count);
        free(reports);
      };
      try {
        _rsl_test_run_with_coverage(run_test,
                                    static_cast<void const*>(&fnc),
                                    &reports,
                                    &report_count);
        finalize();
      } catch (...) { 
        finalize();
        throw;
      }
    } else {
      fnc();
    }
    auto t1 = std::chrono::steady_clock::now();

    ret.outcome     = TestOutcome(!test->expect_failure);
    ret.duration_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return ret;
  } catch (assertion_failure const& failure) {
    ret.failure = failure;
  } catch (std::exception const& exc) {  //
    ret.exception += exc.what();
  } catch (std::string const& msg) {  //
    ret.exception += msg;
  } catch (std::string_view msg) {  //
    ret.exception += msg;
  } catch (char const* msg) {  //
    ret.exception += msg;
  } catch (...) { ret.exception += "unknown exception thrown"; }

  ret.outcome = TestOutcome(test->expect_failure);
  return ret;
}
}  // namespace rsl::testing