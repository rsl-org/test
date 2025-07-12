#include <rsl/test>
#include <string_view>
#include <algorithm>

// https://docs.pytest.org/en/6.2.x/fixture.html

namespace demo::fixtures {
[[=rsl::fixture]] 
std::string_view my_fruit() {
  return "apple";
}

[[=rsl::fixture]]
std::vector<std::string_view> fruit_basket(std::string_view my_fruit) {
  return {"banana", my_fruit};
}

[[=rsl::test]] 
void test_my_fruit_in_basket(std::string_view my_fruit, 
                             std::vector<std::string_view> fruit_basket) {
  ASSERT(std::ranges::contains(fruit_basket, my_fruit));
};
}  // namespace
