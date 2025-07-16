#include <print>
#include <string>
#include <optional>
#include <vector>

#include <rsl/testing/output.hpp>
#include <rsl/xml>
#include "rsl/testing/annotations.hpp"

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

struct Section {
  [[= xml::attribute]] std::string name;
  [[= xml::attribute]] std::optional<std::string> filename;
  [[= xml::attribute]] std::optional<unsigned> line;
  std::vector<Section> sections;
  OverallResults results;

  Section& get_section(std::string_view name) {
    for (auto& section : sections) {
      if (section.name == name) {
        return section;
      }
    }
    sections.emplace_back(std::string(name));
    return sections.back();
  }

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

  std::vector<Section> sections;
  OverallResult result;

  Section& get_section(std::string_view name) {
    for (auto& section : sections) {
      if (section.name == name) {
        return section;
      }
    }
    sections.emplace_back(std::string(name));
    return sections.back();
  }

  void update_results() {
    for (auto& section : sections) {
      section.update_results();
    }
  }
};

struct Catch2TestRun {
  [[= xml::attribute]] std::optional<std::string> name;
  [[= xml::attribute]] std::optional<std::size_t> rng_seed;
  [[= xml::attribute]] std::size_t xml_format_version = 3;
  [[= xml::attribute]] std::string catch2_version     = "3.8.1";

  std::vector<TestCase> tests;
  OverallResults test_results;
  OverallResultsCases case_results;

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

struct MatchingTests {
  struct TestCase {
    [[= xml::raw]] std::string Name;
  };
  std::vector<MatchingTests::TestCase> tests;
};

class[[= annotations::rename("xml")]] Catch2XmlReporter : public Reporter {
  Catch2TestRun report;
  Section* section;

public:
  void before_run(TestNamespace const&) override {}
  void enter_namespace(std::string_view name) override {}
  void exit_namespace(std::string_view name) override {}
  void before_test_group(Test const& test) override {}
  void after_test_group(std::span<TestResult> results) override {}

  void before_test(Test::TestRun const& run) override {
    TestCase& tc = report.get_tc(run.test->full_name[0]);
    if (run.test->full_name.size() >= 2) {
      section         = &tc.sections.emplace_back(run.test->full_name[1]);
      int single_test = run.test->get_tests().size() == 1;
      for (auto idx = 2; idx < run.test->full_name.size() - single_test; ++idx) {
        section = &section->sections.emplace_back(run.test->full_name[idx]);
      }
      section->sections.push_back({.name     = run.name,
                                   .filename = run.test->sloc.file_name(),
                                   .line     = run.test->sloc.line()});
      section = &section->sections.back();
    } else {
      tc.sections.push_back({.name     = run.name,
                             .filename = run.test->sloc.file_name(),
                             .line     = run.test->sloc.line()});
      section = &tc.sections.back();
    }
  }

  void after_test(TestResult const& result) override {
    if (result.passed) {
      ++section->results.successes;
    } else {
      ++section->results.failures;
    }
    section->results.durationInSeconds += result.duration_ms / 1000.;

    // <Expression success="false" type="REQUIRE"
    // filename="/home/che/src/scratchpad/catch_test/src/test.cpp" line="16">
    //   <Original>
    //     false
    //   </Original>
    //   <Expanded>
    //     false
    //   </Expanded>
    // </Expression>
  }

  void after_run(std::span<TestResult> results) override { report.update_results(); }

  void list_tests(TestNamespace const& tests) override {
    MatchingTests matching{};
    for (auto const& ns : tests.children) {
      matching.tests.emplace_back(std::string(ns.name));
    }

    for (auto const& test : tests.tests) {
      matching.tests.emplace_back(std::string(test.name));
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

REGISTER_REPORTER(Catch2XmlReporter);
}  // namespace rsl::testing::_xml_impl