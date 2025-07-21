#include <rsl/test>
#include <iostream>

namespace demo {

[[=rsl::test]] 
void always_passes() {
  std::cout << "foo\n";
  std::cerr << "bar\n";
  ASSERT(false, "testing");
}

[[=rsl::test, =rsl::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace testing