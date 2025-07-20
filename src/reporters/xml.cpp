#include <rsl/serializer/xml/annotations.hpp>
#include <vector>
#include <string>

#include <rsl/testing/output.hpp>
#include <rsl/xml>

namespace rsl::testing::_impl {
struct testcase {
  [[=xml::attribute]] std::string name;
  [[=xml::attribute]] double time;
  [[=xml::raw]] std::optional<std::string> failure;
};

struct testsuite {
  std::vector<testcase> tests;
};

class [[=rename("junit")]] JUnitXmlReporter : public Reporter::Registrar<JUnitXmlReporter> {
  testsuite suite;
public:
  void before_test(TestRun const& test) override {}
  void after_test(TestResult const& result) override {
    auto node = testcase{.name=std::string(result.name), .time=result.duration_ms / 1000.};
    if (!result.passed) {
      node.failure = result.error + "\n";
    }
    suite.tests.push_back(node);
  }

  void finalize(Output& target) override { target.print(rsl::to_xml(suite)); }
};
}  // namespace rsl::testing::_impl