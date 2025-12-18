#include <rsl/test>
#include "include/dep.h"
#include <iostream>

namespace demo {
auto zoinks(bool zoinks) {
  bool x = true;
  if (zoinks) {
    for (int i = 0; i < 4; ++i) {
      x += std::puts("foo");
    }
  } else {
    x = false;
  }
  ASSERT(zoinks == false);
  return x;
}

[[= rsl::test]] void always_passes() {
  // std::cout << "foo\n";
  // std::cerr << "bar\n";
  zoinks(false);
  // zoinks(true);
  zoinks(foo());
}
}  // namespace demo