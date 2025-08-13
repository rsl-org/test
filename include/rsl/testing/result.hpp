#pragma once
#include <cstddef>
#include <string>

#include "assert.hpp"

namespace rsl::testing {

enum class TestOutcome: uint8_t {
  FAIL,
  PASS,
  SKIP
};

struct LineCoverage {
  std::size_t line = 0;
  std::size_t count = 0;
};

struct FileCoverage {
  std::string filename;
  std::vector<LineCoverage> coverage;
};

struct Result {
  class Test const* test;
  std::string name;

  TestOutcome outcome;
  double duration_ms;

  std::optional<assertion_failure> failure;
  std::string exception;
  std::string stdout;
  std::string stderr;
  
  std::vector<AssertionInfo> assertions;
  std::vector<FileCoverage> coverage;
};

struct TestResult {
  class Test const* test;
  std::vector<Result> results;
};
}  // namespace rsl::testing