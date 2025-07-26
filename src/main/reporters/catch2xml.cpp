#include <numeric>
#include <print>
#include <string>
#include <optional>
#include <vector>

#include <rsl/testing/output.hpp>
#include <rsl/xml>

namespace rsl::testing::_xml_impl {
struct OverallResult {
  [[= xml::attribute]] bool success             = true;
  [[= xml::attribute]] unsigned skips           = 0;
  [[= xml::attribute]] double durationInSeconds = 0.0;
};

struct OverallResults {
  [[= xml::attribute]] unsigned successes        = 0;
  [[= xml::attribute]] unsigned failures         = 0;
  [[= xml::attribute]] unsigned expectedFailures = 0;
  [[= xml::attribute]] bool skipped              = false;
  [[= xml::attribute]] double durationInSeconds  = 0.0;
};

struct OverallResultsCases {
  [[= xml::attribute]] unsigned successes        = 0;
  [[= xml::attribute]] unsigned failures         = 0;
  [[= xml::attribute]] unsigned expectedFailures = 0;
  [[= xml::attribute]] unsigned skips            = 0;
};

struct StdOut {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};
struct StdErr {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};
struct Exception {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};
struct FatalErrorCondition {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};
struct Failure {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};
struct Warning {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};
struct Info {
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::raw]] std::string value;
};

struct Section {
  [[= xml::attribute]] std::string name;
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  [[= xml::node]] std::vector<Section> sections;
  [[= xml::node]] OverallResults results;

  [[= xml::node]] std::optional<StdOut> stdout;
  [[= xml::node]] std::optional<StdErr> stderr;
  [[= xml::node]] std::optional<Exception> exception;
  [[= xml::node]] std::optional<FatalErrorCondition> fatal_ec;
  [[= xml::node]] std::optional<Failure> failure;
  [[= xml::node]] std::optional<Warning> warning;
  [[= xml::node]] std::optional<Info> info;

  void update_results() {
    for (auto& section : sections) {
      section.update_results();
      results.successes += section.results.successes;
      results.failures += section.results.failures;
      results.expectedFailures += section.results.expectedFailures;
      results.durationInSeconds += section.results.durationInSeconds;
    }
  }
};

struct TestCase {
  [[= xml::attribute]] std::string name;
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;

  [[= xml::node]] std::vector<Section> sections;
  [[= xml::node]] OverallResult result;

  void update_results() {
    for (auto& section : sections) {
      section.update_results();

      // TODO this is no good, we need to count skips in sections
      result.skips += unsigned(section.results.skipped);
    }
    result.success = std::ranges::none_of(sections, [](Section const& section) {
      return section.results.failures > section.results.expectedFailures;
    });
  }
};

struct Catch2TestRun {
  [[= xml::attribute]] std::optional<std::string> name;
  [[= xml::attribute]] std::optional<std::size_t> rng_seed;
  [[= xml::attribute]] std::size_t xml_format_version = 3;
  [[= xml::attribute]] std::string catch2_version     = "3.8.1";

  [[= xml::node]] std::vector<TestCase> tests;
  [[= xml::node]] OverallResults test_results;
  [[= xml::node]] OverallResultsCases case_results;

  TestCase& get_tc(std::string_view name) {
    for (auto& test : tests) {
      if (test.name == name) {
        return test;
      }
    }
    tests.emplace_back(std::string(name));
    return tests.back();
  }

  void update_results() {
    test_results = {};
    case_results = {};
    for (TestCase& test : tests) {
      test.update_results();
      if (test.result.success) {
        ++test_results.successes;
      } else {
        ++test_results.failures;
      }
      test_results.durationInSeconds += test.result.durationInSeconds;

      for (Section const& section : test.sections) {
        case_results.successes += section.results.successes;
        case_results.failures += section.results.failures;
        case_results.expectedFailures += section.results.expectedFailures;
        // TODO count skips
      }
    }
  }
};

struct Name {
  [[= xml::raw]] std::string value;
};

struct MatchingTests {
  struct TestCase {
    Name name;
  };
  [[= xml::node]] std::vector<MatchingTests::TestCase> tests;
};

class[[= rename("xml")]] Catch2XmlReporter : public Reporter::Registrar<Catch2XmlReporter> {
  Catch2TestRun report;

public:
  void before_test(rsl::testing::TestCase const& run) override {}
  void after_test(TestResult const& result) override {
    TestCase& tc     = report.get_tc(result.test->full_name[0]);
    Section* section = nullptr;

    if (result.test->full_name.size() >= 2) {
      section = &tc.sections.emplace_back(result.test->full_name[1]);
      for (auto const& part : result.test->full_name | std::views::drop(2)) {
        section = &section->sections.emplace_back(part);
      }
      section->sections.emplace_back(result.name,
                                     result.test->sloc.file_name(),
                                     result.test->sloc.line());
      section = &section->sections.back();
    } else {
      tc.sections.emplace_back(result.name,
                               result.test->sloc.file_name(),
                               result.test->sloc.line());
      section = &tc.sections.back();
    }

    if (!result.stdout.empty()) {
      section->stdout = {.value = result.stdout};
    }

    if (!result.stderr.empty()) {
      section->stderr = {.value = result.stderr};
    }

    if (result.passed) {
      ++section->results.successes;
    } else {
      section->failure = {.value = result.failure};
      ++section->results.failures;
    }
    section->results.durationInSeconds += result.duration_ms / 1000.;

    // <Expression success="false" type="REQUIRE"
    // filename="/home/che/src/scratchpad/catch_test/src/test.cpp" line="16">
    //   <Original>
    //     x == false
    //   </Original>
    //   <Expanded>
    //     false == false
    //   </Expanded>
    // </Expression>
  }

  void after_run(std::span<TestResult> results) override { report.update_results(); }

  void list_tests(TestNamespace const& tests) override {
    MatchingTests matching{};
    for (auto const& ns : tests.children) {
      matching.tests.push_back(MatchingTests::TestCase{std::string(ns.name)});
    }

    for (auto const& test : tests.tests) {
      matching.tests.push_back(MatchingTests::TestCase{std::string(test.name)});
    }
    // todo use Output instead
    std::println("{}", rsl::to_xml(matching));
  }

  void finalize(Output& target) override {
    if (!report.tests.empty()) {
      target.print(rsl::to_xml(std::move(report)));
      report = {};
    }
  }
};
}  // namespace rsl::testing::_xml_impl