#pragma once

#include <string>
#include <vector>

namespace cpptest {
struct TestResult {
  std::string name;
  bool passed;
  std::string error;
  double duration_ms;
};

struct Reporter {
  virtual ~Reporter()                                             = default;
  virtual void on_start(size_t total)                             = 0;
  virtual void on_test_start(const std::string& name)             = 0;
  virtual void on_test_end(const TestResult& result)              = 0;
  virtual void on_summary(const std::vector<TestResult>& results) = 0;
  [[nodiscard]] virtual bool colorize() const                     = 0;
};
}  // namespace cpptest