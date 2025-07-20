#include <rsl/testing/_testing_impl/discovery.hpp>

namespace rsl::testing {

std::set<TestDef>& _testing_impl::registry() {
  static std::set<TestDef> data;
  return data;
}
}  // namespace rsl::testing