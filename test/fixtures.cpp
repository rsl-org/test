#include <retest.hpp>
namespace {

[[=rsl::fixture]]
int meta_fixture() { return 21; }

[[=rsl::fixture]]
int fixture(int meta_fixture) { return meta_fixture * 2; }

[[=rsl::test]]
void test_with_fixture(int fixture){
    ASSERT(fixture > 5);
};
}  // namespace
