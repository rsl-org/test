#define RETEST_SKIP
#include <retest.hpp>

namespace testing {

[[= retest::test]] 
void always_passes() {}

}  // namespace testing

RETEST_ENABLE_NS(testing)