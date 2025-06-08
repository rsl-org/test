#include <cpptest.hpp>
#include <print>


namespace cpptest {
std::vector<Test>& registry() {
  static std::vector<Test> test_registry;
  return test_registry;
}

void run_tests() {
  std::println("Found {} tests", registry().size());

  for (auto test : registry()) {
    bool result = test.run();
    std::println("Test {} -> {}", test.name, result);
  }
}

}  // namespace cpptest