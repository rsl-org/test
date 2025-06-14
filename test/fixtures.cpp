#include <retest.hpp>
namespace {

[[=re::fixture]]
int meta_fixture() { return 21; }

[[=re::fixture]]
int fixture(int meta_fixture) { return meta_fixture * 2; }

[[=re::test]]
void test_with_fixture(int fixture){
    ASSERT(fixture > 5);
};
}  // namespace
