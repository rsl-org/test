#include <rsl/testing/discovery.hpp>
#include "rsl/testing/output.hpp"

namespace rsl::testing {

std::set<TestDef>& _testing_impl::registry() {
  static std::set<TestDef> data;
  return data;
}
}  // namespace rsl::testing