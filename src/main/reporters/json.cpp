#include <rsl/testing/output.hpp>
#include <print>
#include <rsl/testing/assert.hpp>
#include "rsl/testing/result.hpp"

namespace rsl::testing::_impl {
class[[= rename("json")]] JsonReporter : public Reporter::Registrar<JsonReporter> {
public:
  void before_run(TestNamespace const& tests) override {}
  void enter_namespace(std::string_view name) override {}
  void before_test_group(Test const& test) override {}
  void before_test(TestCase const& test) override {}
  void after_test(Result const& result) override {}
  void after_test_group(std::span<Result> results) override {}
  void exit_namespace(std::string_view name) override {}
  void after_run() override {}
};
}  // namespace rsl::testing::_impl
