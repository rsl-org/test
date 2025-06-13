#define CPPTEST_SKIP
#include <cpptest.hpp>

namespace testing {

[[= cpptest::test]] 
void always_passes() {}

}  // namespace testing

CPPTEST_ENABLE_NS(testing)