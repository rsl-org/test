#include <rsl/test>
#include <cstdlib>


namespace {
bool foo() {
  return false;
}

[[= rsl::test]] void always_passes() {
  ASSERT(!foo());
}
}  // namespace

