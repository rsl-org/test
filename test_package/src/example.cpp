#include <retest.hpp>

namespace {
[[= retest::test]] void always_passes() {}
}  // namespace

int main() {
    retest::run_tests();
}
