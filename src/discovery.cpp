#include <rsl/testing/discovery.hpp>

std::set<rsl::testing::TestDef>& rsl::testing::registry() {
  static std::set<TestDef> test_registry;
  return test_registry;
}
