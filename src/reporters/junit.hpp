#pragma once
#include <rsl/testing/output.hpp>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

namespace rsl::testing::_impl {

struct Attribute {
  std::string_view name;
  std::string_view value;
};

class Section {
  Output* out;
  std::string name;

public:
  explicit Section(Output* out_, std::string_view name, std::vector<Attribute> attributes = {})
      : out(out_)
      , name(name) {
    out->printf("{}<{}", out->indent(), name);
    for (auto const& attribute : attributes) {
      out->printf(R"( {}="{}")", attribute.name, attribute.value);
    }
    out->print(">\n");
    out->push_level();
  }
  ~Section() {
    out->pop_level();
    out->printf("{}</{}>\n", out->indent(), name);
  }
};

struct XmlReporter : Reporter {
  /// Write XML prologue/header to the provided stream
  explicit XmlReporter(Output* out_) : out(out_) {
    out->print(R"(<?xml version="1.0" encoding="UTF-8"?>)"
               "\n");
  }

  virtual ~XmlReporter() override {}
  void push_section(std::string_view name, std::vector<Attribute> attributes = {}) {
    sections.emplace(out, name, attributes);
  }
  void pop_section() { sections.pop(); }

private:
  std::stack<Section> sections;

protected:
  Output* out;
};

struct JUnitXmlReporter : XmlReporter {
  explicit JUnitXmlReporter(Output* out) : XmlReporter(out) {}

  void before_run(TestNamespace const&) override { push_section("testsuite"); }

  void before_test(Test const& test) override {}
  void after_test(TestResult const& result) override {
    Section _{
        out,
        "testcase",
        {{"name", result.name}, {"time", std::format("{:.3f}", result.duration_ms / 1000.)}}
    };

    if (!result.passed) {
      Section _{out, "failure"};
      out->print(result.error);
      out->print("\n");
    }
  }

  void after_run(std::vector<TestResult> const& results) override {}
};

}  // namespace rsl::testing::_impl