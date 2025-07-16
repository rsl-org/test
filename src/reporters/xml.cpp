#include <numeric>
#include <stdexcept>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>

#include <rsl/testing/output.hpp>

namespace rsl::testing::_impl {
struct XmlNode {
  std::string tag;
  std::unordered_map<std::string, std::string> attributes;
  std::vector<XmlNode> children;
  std::string raw_content;
  XmlNode* parent = nullptr;

  XmlNode() = default;
  XmlNode(std::string tag, std::unordered_map<std::string, std::string> attributes = {}, std::vector<XmlNode> children = {})
    : tag(std::move(tag)), attributes(std::move(attributes)), children(std::move(children)) {
    for (auto& child : this->children)
      child.parent = this;
  }

  XmlNode* up() {
    return parent;
  }

  XmlNode& add(XmlNode child) {
    child.parent = this;
    children.push_back(std::move(child));
    return children.back();
  }

  XmlNode* find(std::function<bool(const XmlNode&)> pred) {
    if (pred(*this)) return this;
    for (auto& child : children)
      if (auto* found = child.find(pred))
        return found;
    return nullptr;
  }

  bool has_attribute(std::string_view str, std::string_view value = {}) const {
    auto it = attributes.find(std::string(str));
    if (it == attributes.end()) {
      return false;
    }
    return value.empty() ? true : it->second == value;
  }

  std::string stringify(std::size_t indent_level = 0) const {
    // if (tag.empty()) {
    //   return "";
    // }
    std::ostringstream result;
    if (parent == nullptr) {
      // at document root, output xml meta
      result << R"(<?xml version="1.0" encoding="UTF-8"?>)""\n";
    }

    auto indent = std::string(indent_level * 2, ' ');
    result << indent << '<' << tag;
    for (auto const& [name, value] : attributes) {
      result << ' ' << name << "=\"" << value << '"';
    }

    if (children.empty() && raw_content.empty()) {
      result << "/>";
      return result.str();
    }

    result << '>';
    if (raw_content.empty()) {
      for (auto const& child : children) {
        result << '\n' << child.stringify(indent_level + 1);
      }
      result << '\n' << indent;
    } else {
      result << raw_content;
      if (!children.empty()) {
        throw std::runtime_error(
            "Cannot have raw_content and child nodes at the same time");
      }

      if (raw_content.back() == '\n') {
        result << indent;
      }
    }

    result << "</" << tag << '>';
    return result.str();
  }
};

class [[=annotations::rename("junit")]] JUnitXmlReporter : public Reporter {
  XmlNode document;
  XmlNode* cursor;
public:
  void before_run(TestNamespace const&) override { 
    document.tag = "testsuite";
  }

  void before_test(Test::TestRun const& test) override {}

  void after_test(TestResult const& result) override {
    auto& section = document.add({"testcase"});
    section.attributes["name"] = result.name;
    section.attributes["time"] = std::format("{:.3f}", result.duration_ms / 1000.);

    if (!result.passed) {
      auto& failure = section.add({"failure"});
      failure.raw_content = result.error + "\n";
    }
  }

  void after_run(std::span<TestResult> results) override { }

  void finalize(Output& target) override { target.print(document.stringify()); }
};

REGISTER_REPORTER(JUnitXmlReporter);

// class Catch2XmlReporter : public Reporter {
//   XmlNode document;
//   XmlNode* tc = nullptr;

// public:
//   void before_run(TestNamespace const&) override { 
//     document.tag = "Catch2TestRun";
//   }

//   void enter_namespace(std::string_view name) override {}

//   void exit_namespace(std::string_view name) override {}

//   void before_test_group(Test const& test) override {
    
//   }

//   void after_test_group(std::span<TestResult> results) override {
    
//   }

//   void before_test(Test::TestRun const& run) override {
//     Test const& test = *run.test;
//     tc = document.find([&](XmlNode const& node) {
//       return node.tag == "TestCase" && node.has_attribute("name", test.full_name[0]);
//     });

//     if (tc == nullptr) {
//       // <TestCase name="demo" filename="/home/che/src/scratchpad/catch_test/src/test.cpp" line="10">
//       XmlNode& node           = document.add({"TestCase"});
//       node.attributes["name"] = test.full_name[0];
      
//       // <OverallResult success="true" skips="0" durationInSeconds="0.000188"/>
//       XmlNode& result = node.add({"OverallResult"});
//       result.attributes["success"] = "true";
//       result.attributes["skips"] = "0";

//       //<StdOut></StdOut>
//       //<StdErr></StdErr>

//       tc                      = &node;
//     }

//     for (auto const& part : test.full_name | std::views::drop(1)) {
//       XmlNode& node           = tc->add({"Section"});
//       node.attributes["name"] = part;

//       // <OverallResults successes="0" failures="0" expectedFailures="0" skipped="false" durationInSeconds="3.2e-05"/>
//       XmlNode& result = node.add({"OverallResults"});
//       result.attributes["successes"] = "0";
//       result.attributes["failures"] = "0";
//       result.attributes["expectedFailures"] = "0";
//       result.attributes["successes"] = "false";

//       tc                      = &node;
//     }

//     XmlNode& node               = tc->add({"Section"});
//     node.attributes["filename"] = run.test->sloc.file_name();
//     node.attributes["line"]     = std::to_string(run.test->sloc.line());
//     node.attributes["name"]     = run.name;
//   }

//   void after_test(TestResult const& result) override {
//     auto& node                              = *tc;
//     auto& results                           = node.add({"OverallResults"});
//     results.attributes["successes"]         = result.passed ? "1" : "0";
//     results.attributes["failures"]          = result.passed ? "0" : "1";
//     results.attributes["durationInSeconds"] = std::format("{:.3f}", result.duration_ms / 1000.);

//     // <Expression success="false" type="REQUIRE" filename="/home/che/src/scratchpad/catch_test/src/test.cpp" line="16">
//     //   <Original>
//     //     false
//     //   </Original>
//     //   <Expanded>
//     //     false
//     //   </Expanded>
//     // </Expression>
//   }

//   void after_run(std::span<TestResult> results) override { 
//     // <OverallResults successes="0" failures="0" expectedFailures="0" skips="0"/>
//     // <OverallResultsCases successes="1" failures="0" expectedFailures="0" skips="0"/>
//     auto& node = document.add({"OverallResults"});
//     node.attributes["successes"] = "0";
//     node.attributes["failures"] = "0";
//     node.attributes["expectedFailures"] = "0";
//     node.attributes["skips"] = "0";

//     auto& cases = document.add({"OverallResultsCases"});
//     cases.attributes["successes"] = "1";
//     cases.attributes["failures"] = "0";
//     cases.attributes["expectedFailures"] = "0";
//     cases.attributes["skips"] = "0";
//   }

//   void list_tests(TestNamespace const& tests) override {
//     document.tag = "MatchingTests";

//     auto push_section = [&](std::string_view name) {
//       auto& tc        = document.add({"TestCase"});
//       auto& name_node = tc.add({"Name"});
//       name_node.raw_content = std::string(name);
//     };

//     for (auto const& ns : tests.children) {
//       push_section(ns.name);
//     }

//     for (auto const& test : tests.tests) {
//       push_section(test.name);
//     }

//     // for (auto const& test : tests) {
//     //   push_section(join(test.full_name, "::"));
//     // }
//   }

//   void finalize(Output& target) override { target.print(document.stringify()); }
// };
}  // namespace rsl::testing::_impl