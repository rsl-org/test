#include <cassert>
#include <print>
#include <ranges>
#include <vector>

#include <cpptrace/basic.hpp>
#include <cpptrace/utils.hpp>

#include <rsl/test>
#include <rsl/testing/_testing_impl/discovery.hpp>

#include <rsl/testing/_unit_impl/capture.hpp>
#include <rsl/testing/_unit_impl/result.hpp>

namespace rsl::testing {
namespace _testing_impl {
std::set<TestDef>& registry() {
  static std::set<TestDef> reg;
  return reg;
}

struct AssertionTracker {
  std::vector<AssertionInfo> assertions;
  std::string test_name;
};

inline AssertionTracker& assertion_counter() {
  static AssertionTracker counter{};
  return counter;
}

void track_assertion(AssertionInfo info) {
  assertion_counter().assertions.emplace_back(info);
}
}  // namespace _testing_impl

Result run(TestCase& tc) {
  auto ret = Result{.test = tc.test, .name = tc.name};
  try {
    Capture _out(stdout, ret.stdout);
    Capture _err(stderr, ret.stderr);

    auto t0 = std::chrono::steady_clock::now();
    // if (_rsl_test_run_with_coverage != nullptr) {
    //   // rsltest_cov was linked in -> run with coverage
    //   rsl::coverage::CoverageReport* reports = nullptr;
    //   std::size_t report_count               = 0;
    //   auto finalize                          = [&] {
    //     ret.coverage = filter_coverage(reports, report_count);
    //     free(reports);
    //   };
    //   try {
    //     _rsl_test_run_with_coverage(_impl::run_test,
    //                                 static_cast<void const*>(&fnc),
    //                                 &reports,
    //                                 &report_count);
    //     finalize();
    //   } catch (...) {
    //     finalize();
    //     throw;
    //   }
    // } else {
    tc.fnc();
    // }
    auto t1 = std::chrono::steady_clock::now();

    ret.outcome     = TestOutcome(!tc.test->expect_failure);
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

  ret.outcome = TestOutcome(tc.test->expect_failure);
  return ret;
}

}  // namespace rsl::testing

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

void list_tests() {
  for (auto def : rsl::testing::_testing_impl::registry()) {
    auto tests = def("");
    std::println("test: {}", tests.name);
  }
}

void run_tests(std::vector<std::string_view> filters) {
  std::println("test, filters {}", filters);
  libassert::set_failure_handler(failure_handler);

  for (auto def : rsl::testing::_testing_impl::registry()) {
    auto tests = def("");

    rsl::testing::_testing_impl::assertion_counter().test_name =
        tests.full_name | std::views::transform([](auto c) { return std::string(c); }) |
        std::views::join_with(std::string("::")) | std::ranges::to<std::string>();

    std::println("{}", tests.full_name);
    for (auto tc : tests.get_tests()) {
      auto result = run(tc);
      std::println("->{}", int(result.outcome));
      for (auto assertion : result.assertions) {
        std::println("{} -> {} = {}", assertion.raw, assertion.expanded, assertion.success);
      }
      if (result.failure) {
        std::println("{}", (*result.failure).message);
      }
    }

    rsl::testing::_testing_impl::assertion_counter().test_name = "";
  }

  libassert::set_failure_handler(libassert::default_failure_handler);
}
}  // namespace

int main(int argc, char** argv) {
  assert(argc > 0);
  auto& registry = rsl::testing::_testing_impl::registry();

  if (argc == 1) {
    list_tests();
  } else {
    // run tests matching queries
    // `::` matches all tests
    std::vector<std::string_view> filters;
    for (auto idx : std::views::iota(1, argc)) {
      filters.emplace_back(argv[idx]);
    }
    run_tests(filters);
  }
}