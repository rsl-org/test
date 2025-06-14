#include <retest.hpp>

namespace {
[[= re::test]] void always_passes() {}
}  // namespace

int main() {
    re::run_tests();
}
