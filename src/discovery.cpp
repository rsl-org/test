#include <cpptest/discovery.hpp>

std::set<cpptest::TestDef>& cpptest::registry() {
  static std::set<TestDef> test_registry;
  return test_registry;
}
