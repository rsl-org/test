#define RETEST_SKIP
#include <retest.hpp>

namespace testing {

[[= re::test]] 
void always_passes() {}

}  // namespace testing

RETEST_ENABLE_NS(testing)