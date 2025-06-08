#include <cpptest.hpp>

namespace {
[[= cpptest::test]]
[[= cpptest::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace
