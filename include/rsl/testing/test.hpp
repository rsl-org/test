#pragma once
#include <string>
#include <functional>
#include <meta>
#include <iterator>
#include <deque>
#include <algorithm>
#include <filesystem>
#include <vector>

#include "_testing_impl/util.hpp"
#include "_testing_impl/expand.hpp"

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/assert.hpp>

namespace rsl::testing {
struct TestCase {
  class Test const* test;
  std::function<void()> fnc;
  std::string name;
};

class Test {
  using runner_type = std::vector<TestCase> (Test::*)() const;
  runner_type get_tests_impl;

  template <std::meta::info R, _testing_impl::Annotations Ann>
  std::vector<TestCase> expand_test() const {
    return _testing_impl::Expand<TestCase, R, Ann>{this}.runs;
  }

public:
  std::source_location sloc;
  std::string module_path;
  std::string_view name;                   // raw name
  std::string_view preferred_name;         // from annotations
  std::span<char const* const> full_name;  // fully qualified name

  bool expect_failure;  // invert test checking
  bool (*skip)();       // function to support conditional skipping

  Test() = delete;
  consteval explicit Test(std::meta::info test, std::meta::info annotation_anchor)
      : sloc(source_location_of(test))
      , name(define_static_string(identifier_of(test))) {
    auto ann       = _testing_impl::Annotations(annotation_anchor);
    preferred_name = ann.name;
    expect_failure = ann.expect_failure;
    skip           = ann.skip;

    get_tests_impl = extract<runner_type>(
        substitute(^^expand_test, {reflect_constant(test), std::meta::reflect_constant(ann)}));

    std::vector<char const*> meta_name;
    for (auto part : _testing_impl::get_fully_qualified_name(test)) {
      meta_name.push_back(std::define_static_string(part));
    }
    full_name = define_static_array(meta_name);
  }

  std::vector<TestCase> get_tests() const { return (this->*get_tests_impl)(); }
};

using TestDef = Test (*)(std::string const&);
}  // namespace rsl::testing