#include <rsl/testing/ext/output.hpp>
#include <print>
#include <rsl/testing/assert.hpp>
#include "rsl/testing/result.hpp"

#include <meta>
#include <ranges>
#include <nlohmann/json.hpp>

namespace rsl::testing::_impl {

std::string stringify_outcome(TestOutcome outcome) {
  constexpr static auto names =
      std::define_static_array(enumerators_of(^^TestOutcome) | std::views::transform([](auto x) {
                                 return std::define_static_string(identifier_of(x));
                               }));
  return names[(size_t)outcome];
}

class[[= rename("json")]] JsonReporter : public Reporter::Registrar<JsonReporter> {
public:
  void before_run(TestNamespace const& tests) override {}
  void enter_namespace(std::string_view name) override {}
  void before_test_group(Test const& test) override {}
  void before_test(TestCase const& test) override {}
  void after_test(Result const& result) override {}
  void exit_namespace(std::string_view name) override {}

  nlohmann::json output = nlohmann::json::array();
  void after_test_group(std::span<Result> results) override {
    for (auto const& result : results) {
      output.push_back(nlohmann::json({
          {     "name",                       result.name},
          {"full_name",            result.test->full_name},
          { "duration",                result.duration_ms},
          {  "outcome", stringify_outcome(result.outcome)},
          {"exception",                  result.exception}
      }));
    }
  }

  void after_run() override { 
    std::println("{}", nlohmann::json({{"action", "test_result"}, {"results", output}}).dump());
    std::fflush(stdout);
    output.clear();
  }
};
}  // namespace rsl::testing::_impl
