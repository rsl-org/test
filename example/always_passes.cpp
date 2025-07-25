#include <rsl/test>
#include <iostream>

namespace demo {
  template <typename T= void>
auto zoinks()  -> T {
  ASSERT(false, "oh no");
}

[[= rsl::test]] void always_passes() {
  std::cout << "foo\n";
  std::cerr << "bar\n";
  zoinks();
}

[[ = rsl::test, = rsl::expect_failure ]] void always_fails() {
  ASSERT(false, "oh no");
}
}  // namespace demo