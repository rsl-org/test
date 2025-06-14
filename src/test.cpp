
#include <libassert/assert.hpp>
#include <retest/all.hpp>
#include <retest/reporter.hpp>

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
  throw re::assertion_failure(message);
}

namespace re {

bool run(std::vector<Test> const& tests, Reporter& reporter) {
  reporter.on_start(tests.size());
  std::vector<TestResult> results;

  libassert::set_failure_handler(reporter.colorize() ? failure_handler<true>
                                                     : failure_handler<false>);
  for (auto const& test : tests) {
    reporter.on_test_start(test.name);
    for (auto const& test_run : test.get_tests()) {
      auto result = test_run.run();
      reporter.on_test_end(result);
      results.push_back(result);
    }
  }
  libassert::set_failure_handler(libassert::default_failure_handler);

  reporter.on_summary(results);
  return std::ranges::all_of(results, [](auto& r) { return r.passed; });
}

std::vector<Test::TestRun> Test::get_tests() const {
  std::vector<TestRun> tests{};
  for (auto fnc : run_fncs){
    tests.append_range((this->*fnc)());
  }
  return tests;
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
}  // namespace re