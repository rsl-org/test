#include <retest.hpp>

namespace foo {
[[= retest::test]]
[[= retest::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace foo
