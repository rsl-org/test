#include <rsl/test>
#include <iostream>

namespace demo {
auto zoinks(bool zoinks) {
  bool x = true;
  // ASSERT(x == false);
  if (zoinks) {
    for (int i = 0; i < 4; ++i) {
      x += std::puts("foo");
    }
  } else {
    x = false;
  }
  return x;
}

[[= rsl::test]] void always_passes() {
  std::cout << "foo\n";
  std::cerr << "bar\n";
  zoinks(true);
  zoinks(false);
}
}  // namespace demo