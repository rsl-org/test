#pragma once
#include <retest/reporter.hpp>
#include <array>
#include <print>
#include <algorithm>
#include <libassert/assert.hpp>

namespace re::_impl {
class ConsoleReporter : public Reporter {
public:
  void on_start(size_t total) override { std::print("Running {} tests...\n", total); }
  void on_test_start(std::string_view name) override { std::print("[ RUN      ] {}\n", name); }
  void on_test_end(TestResult const& result) override {
    bool const must_colorize = colorize();
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
    }
  }
  void on_summary(const std::vector<TestResult>& results) override {
    auto passed = std::ranges::count_if(results, [](auto& r) { return r.passed; });
    std::print("=== Summary ===\n{} / {} tests passed.\n", passed, results.size());
  }

  [[nodiscard]] bool colorize() const override {
    return libassert::isatty(libassert::stderr_fileno);
  }
};
}  // namespace re::_impl