#include <rsl/testing/discovery.hpp>

std::set<rsl::testing::TestDef>& rsl::testing::_testing_impl::registry() {
  static std::set<TestDef> test_registry;
  return test_registry;
}
