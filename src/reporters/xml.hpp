#pragma once
#include <numeric>
#include <stdexcept>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <iomanip>
#include <sstream>

#include <rsl/testing/output.hpp>

namespace rsl::testing::_impl {
class XmlNode {
  std::vector<XmlNode> children;
  std::string raw_content;

public:
  std::string name;
  std::unordered_map<std::string, std::string> attributes;

  explicit XmlNode(std::string_view name) : name(std::string(name)) {}

  XmlNode& add(std::string const& section_name) {
    children.emplace_back(section_name);
    return children.back();
  }

  XmlNode& add(XmlNode const& section) {
    children.push_back(section);
    return children.back();
  }

  [[nodiscard]]
  bool has_attribute(std::string_view name, std::string_view value) const {
    return std::ranges::any_of(attributes, [&](auto const& attr) {
      return attr.first == name && attr.second == value;
    });
  }

  XmlNode* select(auto filter) {
    for (auto& node : children) {
      if (filter(node)) {
        return &node;
      }
    }
    return nullptr;
  }

  void add_raw(std::string const& content) { raw_content += content; }

  XmlNode& operator[](std::string const& section_name) {
    for (auto& child : children) {
      if (child.name == section_name) {
        return child;
      }
    }
    children.emplace_back(section_name);
    return children.back();
  }

  std::string stringify(std::size_t indent_level = 0) const {
    if (name.empty()) {
      return "";
    }
    std::ostringstream result;
    auto indent = std::string(indent_level * 2, ' ');
    result << indent << '<' << name;
    for (auto const& [name, value] : attributes) {
      result << ' ' << name << '=' << std::quoted(value);
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
        throw std::runtime_error("Cannot have raw_content and child nodes at the same time");
      }

      if (raw_content.back() == '\n') {
        result << indent;
      }
    }

    result << "</" << name << '>';
    return result.str();
  }
};

class XML {
  XmlNode root;
  std::stack<XmlNode*> stack;

public:
  XmlNode& current() {
    if (stack.empty()) {
      stack.push(&root);
      return root;
    }
    return *stack.top();
  }

  XmlNode& push(std::string const& section_name) {
    if (stack.empty()) {
      if (section_name != root.name) {
        if (!root.name.empty()) {
          throw std::runtime_error("Can only have one document root");
        }
        root.name = section_name;
      }

      stack.push(&root);
      return root;
    }
    auto& it = current()[section_name];
    stack.push(&it);
    return it;
  }

  XmlNode& add(std::string const& section_name) {
    auto& it = current().add(section_name);
    stack.push(&it);
    return it;
  }

  XmlNode* select(auto filter) { 
    return current().select(std::move(filter));
  }

  void move_cursor(XmlNode* node) {
    if (node != nullptr) {
      stack.push(node);
    }
  }

  void pop() {
    if (stack.size() > 0) {
      stack.pop();
    }
  }

  [[nodiscard]] bool at_root() const { return stack.size() <= 1; }

  std::string stringify() const {
    auto tree = root.stringify();
    if (!tree.empty()) {
      return R"(<?xml version="1.0" encoding="UTF-8"?>)"
             "\n" +
             tree;
    }
    return "";
  }

  XML() : root{""} {}
};

class JUnitXmlReporter : public Reporter {
  _impl::XML document;

public:
  void before_run(TestNamespace const&) override { document.push("testsuite"); }

  void before_test(Test::TestRun const& test) override {}

  void after_test(TestResult const& result) override {
    auto& section = document.add("testcase");
    ;
    section.attributes["name"] = result.name;
    section.attributes["time"] = std::format("{:.3f}", result.duration_ms / 1000.);

    if (!result.passed) {
      auto failure = section.add("failure");
      failure.add_raw(result.error + "\n");
    }
    document.pop();
  }

  void after_run(std::span<TestResult> results) override { document.pop(); }

  void finalize(Output& target) override { target.print(document.stringify()); }
};

template <std::ranges::range R>
std::string join(R&& values, std::string_view delimiter) {
  auto fold = [&](std::string a, auto b) { return std::move(a) + delimiter + b; };

  return std::accumulate(std::next(values.begin()), values.end(), std::string(values[0]), fold);
}

class Catch2XmlReporter : public Reporter {
  _impl::XML document;

public:
  void before_run(TestNamespace const&) override { document.push("Catch2TestRun"); }

  void enter_namespace(std::string_view name) override {}

  void exit_namespace(std::string_view name) override {}

  void before_test_group(Test const& test) override {
    
  }

  void after_test_group(std::span<TestResult> results) override {
    
  }

  void before_test(Test::TestRun const& run) override {
    Test const& test = *run.test;
    XmlNode* tc = document.select([&](XmlNode& node) {
      return node.name == "TestCase" && node.has_attribute("name", test.full_name[0]);
    });

    if (tc == nullptr) {
      // <TestCase name="demo" filename="/home/che/src/scratchpad/catch_test/src/test.cpp" line="10">
      XmlNode& node           = document.current().add("TestCase");
      node.attributes["name"] = test.full_name[0];
      
      // <OverallResult success="true" skips="0" durationInSeconds="0.000188"/>
      XmlNode& result = node.add("OverallResult");
      result.attributes["success"] = "true";
      result.attributes["skips"] = "0";

      //<StdOut></StdOut>
      //<StdErr></StdErr>

      tc                      = &node;
    }

    for (auto const& part : test.full_name | std::views::drop(1)) {
      XmlNode& node           = tc->add("Section");
      node.attributes["name"] = part;

      // <OverallResults successes="0" failures="0" expectedFailures="0" skipped="false" durationInSeconds="3.2e-05"/>
      XmlNode& result = node.add("OverallResults");
      result.attributes["successes"] = "0";
      result.attributes["failures"] = "0";
      result.attributes["expectedFailures"] = "0";
      result.attributes["successes"] = "false";

      tc                      = &node;
    }
    document.move_cursor(tc);

    XmlNode& node               = document.add("Section");
    node.attributes["filename"] = run.test->sloc.file_name();
    node.attributes["line"]     = std::to_string(run.test->sloc.line());
    node.attributes["name"]     = run.name;
  }

  void after_test(TestResult const& result) override {
    auto& node                              = document.current();
    auto& results                           = node.add("OverallResults");
    results.attributes["successes"]         = result.passed ? "1" : "0";
    results.attributes["failures"]          = result.passed ? "0" : "1";
    results.attributes["durationInSeconds"] = std::format("{:.3f}", result.duration_ms / 1000.);

    // <Expression success="false" type="REQUIRE" filename="/home/che/src/scratchpad/catch_test/src/test.cpp" line="16">
    //   <Original>
    //     false
    //   </Original>
    //   <Expanded>
    //     false
    //   </Expanded>
    // </Expression>

    document.pop();
    document.pop();
  }

  void after_run(std::span<TestResult> results) override { 
    // <OverallResults successes="0" failures="0" expectedFailures="0" skips="0"/>
    // <OverallResultsCases successes="1" failures="0" expectedFailures="0" skips="0"/>
    auto& node = document.add("OverallResults");
    node.attributes["successes"] = "0";
    node.attributes["failures"] = "0";
    node.attributes["expectedFailures"] = "0";
    node.attributes["skips"] = "0";
    document.pop(); 

    auto& cases = document.add("OverallResultsCases");
    cases.attributes["successes"] = "1";
    cases.attributes["failures"] = "0";
    cases.attributes["expectedFailures"] = "0";
    cases.attributes["skips"] = "0";
    document.pop();
    document.pop();
  }

  void list_tests(TestNamespace const& tests) override {
    auto& section     = document.push("MatchingTests");
    auto push_section = [&](std::string_view name) {
      auto& tc        = section.add("TestCase");
      auto& name_node = tc.add("Name");
      name_node.add_raw(std::string(name));
    };

    for (auto const& ns : tests.children) {
      push_section(ns.name);
    }

    for (auto const& test : tests.tests) {
      push_section(test.name);
    }

    // for (auto const& test : tests) {
    //   push_section(join(test.full_name, "::"));
    // }
  }

  void finalize(Output& target) override { target.print(document.stringify()); }
};
}  // namespace rsl::testing::_impl