#include <rsl/testing/all.hpp>
#include <iostream>

std::vector<std::tuple<int, char>> bar() { return {{42, 'd'}, {2, 'c'}}; }

consteval std::vector<rsl::testing::ParamSet> foo() { return {{42, '3'}}; }

[[= rsl::tparams{{^^int, 3}}]]
[[= rsl::params{foo}]]
constexpr static auto test =
    []
    <typename T, int V>
    (int, char) static {  };

namespace Zoinks {
template <typename T, int V>
void foo(int, char) {
  std::cout << "\ntest\n";
}
}




int main() {
  // auto runs = expand_test<^^Zoinks::foo, ^^test>();
  // std::cout << '\n' << runs.size();
  // for (auto r : runs) {
  //   std::cout << r.name << '\n';
  // }
}