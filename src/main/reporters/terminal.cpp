#include <rsl/testing/output.hpp>
#include <array>
#include <print>
#include <rsl/testing/assert.hpp>
#include "rsl/testing/result.hpp"

namespace rsl::testing::_impl {
class[[= rename("plain")]] ConsoleReporter : public Reporter::Registrar<ConsoleReporter> {
  std::vector<TestOutcome> test_outcomes;
  std::vector<TestOutcome> run_outcomes;
  std::vector<TestOutcome> assertion_outcomes;
public:
  void before_run(TestNamespace const& tests) override {
    std::print("Running {} tests...\n", tests.count());
  }
  void before_test(TestCase const& test) override { std::print("[ RUN      ] {}\n", test.name); }
  void after_test(Result const& result) override {
    bool const must_colorize = true;
    auto const color = std::array{must_colorize ? "\033[32m" : "", must_colorize ? "\033[31m" : ""};
    const char* const reset = must_colorize ? "\033[0m" : "";

    if (result.outcome == TestOutcome::PASS) {
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
      std::print("{}ERROR{}: {}\n", color[1], reset, result.failure->message);
      std::print("==== {}stdout{} ====\n{}\n", color[1], reset, result.stdout);
      std::print("==== {}stderr{} ====\n{}\n", color[1], reset, result.stderr);
    }
    for (auto const& [file, coverage] : result.coverage) {
      std::println("Reached {} lines in file {}", coverage.size(), file);
    }
    run_outcomes.push_back(result.outcome);
    for (auto const& assertion : result.assertions) {
      assertion_outcomes.push_back(assertion.success ? TestOutcome::PASS : TestOutcome::FAIL);
    }
  }

  void after_test_group(std::span<Result> results) override {
    bool skipped = true;
    bool success = true;
    for (auto const& result : results) {
      if (result.outcome == TestOutcome::SKIP) {
        continue;
      }
      skipped = false;
      success &= result.outcome == TestOutcome::PASS;
    }
    test_outcomes.push_back(skipped ? TestOutcome::SKIP
                                    : (success ? TestOutcome::PASS : TestOutcome::FAIL));
  }

  static void print_outcomes(std::span<TestOutcome> outcomes, std::string_view label) {
    std::size_t fail = 0;
    std::size_t pass = 0;
    std::size_t skip = 0;
    for (auto r : outcomes) {
      switch (r) {
        case TestOutcome::FAIL: ++fail; continue;
        case TestOutcome::PASS: ++pass; continue;
        case TestOutcome::SKIP: ++skip; continue;
      }
    }
    
    bool const must_colorize = true;
    auto const color = std::array{must_colorize ? "\033[32m" : "", must_colorize ? "\033[31m" : ""};
    const char* const reset = must_colorize ? "\033[0m" : "";
    
    std::println("| {:<10} | {}{:^4}{} | {:^4} | {}{:^4}{} || {:^5} |",
      label, color[0], pass, reset, skip, color[1], fail, reset, pass+skip+fail);
  }

  void after_run() override {
    std::println("\n=== Summary ===");
    
    std::puts("+------------+------+------+------++-------+");
    std::puts("|  Counter   | Pass | Skip | Fail || Total |");
    std::puts("+------------+------+------+------++-------+");
    print_outcomes(test_outcomes, "Tests");
    print_outcomes(run_outcomes, "Test Cases");
    print_outcomes(assertion_outcomes, "Assertions");
    std::puts("+------------+------+------+------++-------+");
  }

  // [[nodiscard]] bool colorize() const override {
  //   return libassert::isatty(libassert::stderr_fileno);
  // }
};
}  // namespace rsl::testing::_impl
