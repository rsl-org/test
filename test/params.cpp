#include <cpptest.hpp>
#include <tuple>

namespace {

std::vector<std::tuple<char, int>> make_params() {
    return {{'f', 13}, {'e', 14}};
}

[[=cpptest::test]]
[[=cpptest::params({std::tuple{'a', 10}, {'c', 12}})]]
[[=cpptest::params(cpptest::cartesian_product({'a', 'c'}, {10, 15, 20}))]]
[[=cpptest::params(make_params)]]
void test_with_params(char foo, int bar){
    ASSERT(bar > 5);
    ASSERT(foo != 'x');
};
}  // namespace
