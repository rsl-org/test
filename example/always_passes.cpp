#include <rsl/test>
#include <iostream>

namespace demo {
auto zoinks() {
  ASSERT(false, "oh no");
}

[[= rsl::test]] void always_passes() {
  std::cout << "foo\n";
  std::cerr << "bar\n";
  zoinks();
}
}  // namespace demo