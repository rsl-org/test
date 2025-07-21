#pragma once
#include <functional>

namespace rsl::testing {

struct TestResult {
  class Test const* test;
  std::string name;

  bool passed;
  std::string error;

  std::string stdout;
  std::string stderr;
  double duration_ms;
};

struct TestCase {
  class Test const* test;
  std::function<void()> fnc;
  std::string name;

  [[nodiscard]] TestResult run() const;
};

struct FuzzTarget {
  // stringifying name is pointless here, perhaps do it after failure
  class Test const* test;
  int (*run)(uint8_t const*, size_t);
  size_t (*mutate)(uint8_t*, size_t, size_t, unsigned int);
};
}  // namespace rsl::testing