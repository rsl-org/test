#include <retest.hpp>

namespace foo {
[[= re::test]]
[[= re::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace foo
