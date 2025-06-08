#include <cpptest.hpp>
#include <iostream>

namespace {
[[= cpptest::test]] void always_passes2() {}
}  // namespace

int main() {
    cpptest::run_tests();
}