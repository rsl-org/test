#include <retest.hpp>
#include <tuple>

namespace {

std::vector<std::tuple<char, int>> make_params() {
    return {{'f', 13}, {'e', 14}};
}

[[=rsl::test]]
[[=rsl::params({std::tuple{'a', 10}, {'c', 12}})]]
[[=rsl::params(rsl::cartesian_product({'a', 'c'}, {10, 15, 20}))]]
[[=rsl::params(make_params)]]
void test_with_params(char foo, int bar){
    ASSERT(bar > 5);
    ASSERT(foo != 'x');
};
}  // namespace
