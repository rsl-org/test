#pragma once
#include <functional>
#include <string>
#include "assert.hpp"

namespace rsl::testing {

enum class TestOutcome: uint8_t {
  FAIL,
  PASS,
  SKIP
};

struct TestResult {
  class Test const* test;
  std::string name;

  TestOutcome outcome;
  double duration_ms;

  std::optional<assertion_failure> failure;
  std::string exception;
  std::string stdout;
  std::string stderr;
  
  std::vector<AssertionInfo> assertions;
};

struct ResultNamespace {
  std::string_view name;
  std::vector<TestResult> tests;
  std::vector<ResultNamespace> children;

  void insert(TestResult const& test, size_t i = 0);
  [[nodiscard]] std::size_t count() const;
};
}  // namespace rsl::testing