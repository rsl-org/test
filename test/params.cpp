#include <retest.hpp>
#include <tuple>

namespace {

std::vector<std::tuple<char, int>> make_params() {
    return {{'f', 13}, {'e', 14}};
}

[[=retest::test]]
[[=retest::params({std::tuple{'a', 10}, {'c', 12}})]]
[[=retest::params(retest::cartesian_product({'a', 'c'}, {10, 15, 20}))]]
[[=retest::params(make_params)]]
void test_with_params(char foo, int bar){
    ASSERT(bar > 5);
    ASSERT(foo != 'x');
};
}  // namespace
