#include <rsl/testing/output.hpp>
#include <array>
#include <print>
#include <algorithm>
#include <rsl/testing/assert.hpp>

namespace rsl::testing::_impl {
class [[=rename("plain")]] ConsoleReporter : public Reporter::Registrar<ConsoleReporter> {
public:
  void before_run(TestNamespace const& tests) override { std::print("Running {} tests...\n", tests.count()); }
  void before_test(TestCase const& test) override { std::print("[ RUN      ] {}\n", test.name); }
  void after_test(TestResult const& result) override {
    bool const must_colorize = true;
    auto const color = std::array{must_colorize ? "\033[32m" : "", must_colorize ? "\033[31m" : ""};
    const char* const reset = must_colorize ? "\033[0m" : "";

    if (result.passed) {
      std::print("[{}       OK {}] {} ({:.3f} ms)\n",
                 color[0],
                 reset,
                 result.name,
                 result.duration_ms);
    } else {
      std::print("[{}   FAILED {}] {} ({:.3f} ms)\n",
                 color[1],
                 reset,
                 result.name,
                 result.duration_ms);
      std::print("{}ERROR{}: {}\n", color[1], reset, result.error);
      std::print("==== {}stdout{} ====\n{}\n", color[1], reset, result.stdout);
      std::print("==== {}stderr{} ====\n{}\n", color[1], reset, result.stderr);
    }
  }
  void after_run(std::span<TestResult> results) override {
    auto passed = std::ranges::count_if(results, [](auto& r) { return r.passed; });
    std::print("=== Summary ===\n{} / {} tests passed.\n", passed, results.size());
  }

  // [[nodiscard]] bool colorize() const override {
  //   return libassert::isatty(libassert::stderr_fileno);
  // }
};
}  // namespace rsl::_impl
