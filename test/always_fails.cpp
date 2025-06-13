#include <cpptest.hpp>

namespace foo {
[[= cpptest::test]]
[[= cpptest::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace foo
