#include <retest.hpp>
namespace {

[[=retest::fixture]]
int meta_fixture() { return 21; }

[[=retest::fixture]]
int fixture(int meta_fixture) { return meta_fixture * 2; }

[[=retest::test]]
void test_with_fixture(int fixture){
    ASSERT(fixture > 5);
};
}  // namespace
