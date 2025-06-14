#include <retest/discovery.hpp>

std::set<re::TestDef>& re::registry() {
  static std::set<TestDef> test_registry;
  return test_registry;
}
