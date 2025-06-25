#define RSLTEST_SKIP
#include <rsl/test>

namespace testing {

[[= rsl::test]] 
void always_passes() {}

}  // namespace testing

RSLTEST_ENABLE_NS(testing)