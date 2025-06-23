#include <retest/discovery.hpp>

std::set<rsl::TestDef>& rsl::registry() {
  static std::set<TestDef> test_registry;
  return test_registry;
}
