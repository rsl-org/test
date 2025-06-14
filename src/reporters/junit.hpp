#pragma once
#include <retest/reporter.hpp>
#include <iostream>

namespace re::impl {
class JUnitXmlReporter : public Reporter {
  std::ostream& out_;

public:
  /// Write XML prologue/header to the provided stream
  explicit JUnitXmlReporter(std::ostream& out = std::cout) : out_(out) {
    out_ << R"(<?xml version="1.0" encoding="UTF-8"?>)"
            "\n<testsuite>\n";
  }
  ~JUnitXmlReporter() override { out_ << "</testsuite>\n"; }

  void on_start(size_t) override {}
  void on_test_start(std::string_view name) override {}
  void on_test_end(TestResult const& result) override {
    out_ << std::format(R"(  <testcase name="{}" time="{:.3f}">)",
                        result.name,
                        result.duration_ms / 1000.0);
    if (!result.passed) {
      out_ << "\n    <failure>"  //
           << result.error << "\n"
           << "    </failure>\n  </testcase>\n";
    } else {
      out_ << "</testcase>\n";
    }
  }
  void on_summary(const std::vector<TestResult>&) override {}
  [[nodiscard]] bool colorize() const override { return false; }
};
}  // namespace re::impl