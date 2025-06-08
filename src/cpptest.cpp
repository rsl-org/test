#define CPPTEST_SKIP
#include <cpptest.hpp>
#include <libassert/assert.hpp>
#include <cpptest/reporter.hpp>

std::vector<cpptest::Test>& cpptest::registry() {
  static std::vector<Test> test_registry;
  return test_registry;
}

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
  throw cpptest::assertion_failure(message);
}

bool cpptest::run(std::vector<cpptest::Test> const& tests, cpptest::Reporter& reporter) {
  reporter.on_start(tests.size());
  std::vector<cpptest::TestResult> results;

  libassert::set_failure_handler(reporter.colorize() ? failure_handler<true>
                                                     : failure_handler<false>);
  for (auto& t : tests) {
    reporter.on_test_start(t.name);

    std::string error{};
    auto t0            = std::chrono::steady_clock::now();
    bool ok            = t.run(error);
    auto t1            = std::chrono::steady_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    auto result = cpptest::TestResult{t.name, ok, error, duration_ms};
    reporter.on_test_end(result);
    results.push_back(result);
  }
  libassert::set_failure_handler(libassert::default_failure_handler);

  reporter.on_summary(results);
  return std::ranges::all_of(results, [](auto& r) { return r.passed; });
}
