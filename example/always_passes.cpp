#include <rsl/test>
#include <iostream>

namespace demo {
auto zoinks() {
  bool x = true;
  ASSERT(x == false);
}

[[= rsl::test]] void always_passes() {
  std::cout << "foo\n";
  std::cerr << "bar\n";
  zoinks();
}
}  // namespace demo