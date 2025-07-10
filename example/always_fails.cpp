#include <rsl/test>

namespace foo {
// [[= rsl::test]]
// [[= rsl::expect_failure]]
// void always_fails() {
//     ASSERT(false, "oh no");
// }
}  // namespace foo
