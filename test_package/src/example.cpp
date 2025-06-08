#include <cpptest.hpp>

namespace {
[[= cpptest::test]] void always_passes() {}
}  // namespace

int main() {
    cpptest::run_tests();
}
