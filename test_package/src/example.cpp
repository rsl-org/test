#include <retest.hpp>

namespace {
[[= rsl::test]] void always_passes() {}
}  // namespace

int main() {
    rsl::run_tests();
}
