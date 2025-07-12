#pragma once
#include <cstddef>
#include <string_view>
#include <string>
#include <format>

#include "test.hpp"

namespace rsl::testing {
struct Output {
  virtual ~Output()                        = default;
  virtual void print(std::string_view str) = 0;

  template <typename... T>
  void printf(std::format_string<T...> fmt, T&&... args) {
    print(std::format(fmt, std::forward<T>(args)...));
  }
};

struct Reporter {
  virtual ~Reporter() = default;
  virtual void before_run(TestNamespace const& tests) {}
  virtual void after_run(std::span<TestResult> results) {}

  virtual void before_test_group(Test const& test) {}
  virtual void after_test_group(std::span<TestResult> results) {}

  virtual void before_test(Test::TestRun const& test) = 0;
  virtual void after_test(TestResult const& result)   = 0;

  virtual void list_tests(TestNamespace const& tests);

  virtual void enter_namespace(std::string_view name) {}
  virtual void exit_namespace(std::string_view name) {}

  virtual void finalize(Output& output) {}
};

}  // namespace rsl::testing