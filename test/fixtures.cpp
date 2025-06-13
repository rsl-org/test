#include <cpptest.hpp>
namespace {

[[=cpptest::fixture]]
int meta_fixture() { return 21; }

[[=cpptest::fixture]]
int fixture(int meta_fixture) { return meta_fixture * 2; }

[[=cpptest::test]]
void test_with_fixture(int fixture){
    ASSERT(fixture > 5);
};
}  // namespace
