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

  _impl::XmlNode* current = nullptr;
  void before_test_group(Test const& test) override {
    XmlNode& node           = document.current().add("TestCase");
    node.attributes["name"] = test.full_name[0];
    current                 = &node;
    for (auto const& part : test.full_name | std::views::drop(1)) {
      XmlNode& node           = current->add("Section");
      node.attributes["name"] = part;
      current                 = &node;
    }
  }


  _impl::XmlNode* test = nullptr; // TODO REFACTOR!
  void before_test(Test::TestRun const& run) override {
    XmlNode& node           = current->add("Section");
    node.attributes["filename"] = run.test->sloc.file_name();
    node.attributes["line"] = std::to_string(run.test->sloc.line());
    node.attributes["name"] = run.name;
    test = &node;
  }

  void after_test(TestResult const& result) override {
    auto& node = *test;
    auto& results                           = node.add("OverallResults");
    results.attributes["successes"]         = result.passed ? "1" : "0";
    results.attributes["failures"]          = result.passed ? "0" : "1";
    results.attributes["durationInSeconds"] = std::format("{:.3f}", result.duration_ms / 1000.);
    // document.pop();
    // current = nullptr;
    test = nullptr;
  }

  void after_run(std::span<TestResult> results) override { document.pop(); }

  void list_tests(TestNamespace const& tests) override {
    auto& section     = document.push("MatchingTests");
    auto push_section = [&](std::string_view name) {
      auto& tc        = section.add("TestCase");
      auto& name_node = tc.add("Name");
      name_node.add_raw(std::string(name));
    };

    // for (auto const& ns : tests.children) {
    //   push_section(ns.name);
    // }

    // for (auto const& test : tests.tests) {
    //   push_section(test.name);
    // }
    for (auto const& test : tests) {
      push_section(join(test.full_name, "::"));
    }
  }

  void finalize(Output& target) override { target.print(document.stringify()); }
};
}  // namespace rsl::testing::_impl