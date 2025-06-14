#include <retest.hpp>
#include <tuple>

namespace {

std::vector<std::tuple<char, int>> make_params() {
    return {{'f', 13}, {'e', 14}};
}

[[=re::test]]
[[=re::params({std::tuple{'a', 10}, {'c', 12}})]]
[[=re::params(re::cartesian_product({'a', 'c'}, {10, 15, 20}))]]
[[=re::params(make_params)]]
void test_with_params(char foo, int bar){
    ASSERT(bar > 5);
    ASSERT(foo != 'x');
};
}  // namespace
