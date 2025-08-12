#include <algorithm>
#include <print>

#include <rsl/source_location>
#include <rsl/testing/assert.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/result.hpp>
#include <rsl/testing/output.hpp>
#include <rsl/testing/_testing_impl/discovery.hpp>

namespace rsl::testing {
namespace _testing_impl {
std::set<TestDef>& registry() {
  static std::set<TestDef> data;
  return data;
}

AssertionTracker& assertion_counter() {
  static AssertionTracker counter{};
  return counter;
}
}  // namespace _testing_impl

TestNamespace::iterator::iterator(TestNamespace const& ns) {
  flatten(ns);
  current = elements.front();
  elements.pop_front();
}

void TestNamespace::iterator::flatten(TestNamespace const& current) {
  for (auto const& ns : current.children) {
    flatten(ns);
  }
  if (!current.tests.empty()) {
    elements.push_back({current.tests.begin(), current.tests.end()});
  }
}

TestNamespace::iterator& TestNamespace::iterator::operator++() {
  if (current.it == current.end) {
    if (elements.empty()) {
      current = {};
      return *this;
    }

    current = elements.front();
    elements.pop_front();
  } else {
    ++current.it;
    if (current.it == current.end) {
      return ++*this;
    }
  }
  return *this;
}

bool TestNamespace::iterator::operator==(iterator const& other) const {
  if (current.it != other.current.it || current.end != other.current.end) {
    return false;
  }
  return elements == other.elements;
}

void TestNamespace::insert(Test const& test, std::size_t i) {
  if (i == test.full_name.size() - 1) {
    tests.push_back(test);
    return;
  }

  auto it = std::ranges::find_if(children, [&](const TestNamespace& ns) {
    return ns.name == test.full_name[i];
  });

  if (it == children.end()) {
    children.emplace_back(test.full_name[i]);
    it = std::prev(children.end());
  }

  it->insert(test, i + 1);
}

std::size_t TestNamespace::count() const {
  std::size_t total = tests.size();
  for (auto const& ns : children) {
    total += ns.count();
  }
  return total;
}

void TestNamespace::filter(std::span<std::string const> parts) {
  if (parts.empty()) {
    return;
  }

  std::string_view current          = parts.front();
  std::span<std::string const> next = parts.subspan(1);

  auto it = std::ranges::find_if(children, [&](TestNamespace& ns) { return ns.name == current; });

  if (it != children.end()) {
    tests.clear();
    it->filter(next);
    if (it->children.empty() && it->tests.empty()) {
      children.clear();
    } else {
      children = {*it};
    }
    return;
  } else {
    std::erase_if(tests, [&](const Test& t) { return t.name != current; });
    children.clear();
  }
}

TestRoot get_tests() {
  TestRoot root;
  for (auto test_def : rsl::testing::_testing_impl::registry()) {
    auto test = test_def();
    root.insert(test);
  }
  return root;
}

}  // namespace rsl::testing