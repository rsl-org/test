#include <retest/discovery.hpp>

std::set<retest::TestDef>& retest::registry() {
  static std::set<TestDef> test_registry;
  return test_registry;
}
