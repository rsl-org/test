#include <rsl/test>

namespace demo {

[[=rsl::test]] 
void always_passes() {
  // ASSERT(false, "testing");
}

[[=rsl::test, =rsl::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace testing