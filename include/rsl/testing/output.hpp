#pragma once
#include <cstddef>
#include <string_view>
#include <string>
#include <format>

#include "test.hpp"

namespace rsl::testing {
class Output {
  std::size_t indent_level = 0;

public:
  virtual ~Output()                        = default;
  virtual void print(std::string_view str) = 0;

  void push_level() { ++indent_level; }

  void pop_level() {
    if (indent_level > 0) {
      --indent_level;
    }
  }

  [[nodiscard]] std::string indent() const { return std::string(indent_level * 2, ' '); }

  template <typename... T>
  void printf(std::format_string<T...> fmt, T&&... args) {
    print(std::format(fmt, std::forward<T>(args)...));
  }
};

struct Reporter {
  virtual ~Reporter() = default;
  virtual void before_run(TestNamespace const& tests) {}
  virtual void after_run(std::vector<TestResult> const& results) {}

  virtual void before_test(Test const& test) = 0;
  virtual void after_test(TestResult const& result)   = 0;

  virtual void list_tests(TestNamespace const& tests);

  virtual void enter_namespace(std::string_view name) {}
  virtual void exit_namespace(std::string_view name) {}
};

}  // namespace rsl::testing