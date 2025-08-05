#pragma once
#include <string>
#include <functional>
#include <meta>
#include <iterator>
#include <deque>

#include "result.hpp"

#include "_testing_impl/util.hpp"
#include "_testing_impl/expand.hpp"

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/assert.hpp>

namespace rsl::testing {
struct TestCase {
  class Test const* test;
  std::function<void()> fnc;
  std::string name;

  [[nodiscard]] TestResult run() const;
};

struct FuzzTarget {
  // stringifying name is pointless here, perhaps do it after failure
  class Test const* test;
  int (*run)(uint8_t const*, size_t);
  size_t (*mutate)(uint8_t*, size_t, size_t, unsigned int);
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
  std::string_view name;                   // raw name
  std::string_view preferred_name;         // from annotations
  std::span<char const* const> full_name;  // fully qualified name

  bool expect_failure;  // invert test checking
  bool (*skip)();       // function to support conditional skipping
  bool is_fuzz_test;

  Test() = delete;
  consteval explicit Test(std::meta::info test, std::meta::info annotation_anchor)
      : sloc(source_location_of(test))
      , name(define_static_string(identifier_of(test))) {
    auto ann       = _testing_impl::Annotations(annotation_anchor);
    preferred_name = ann.name;
    expect_failure = ann.expect_failure;
    skip           = ann.skip;
    is_fuzz_test   = ann.is_fuzz_test;

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

using TestDef = Test (*)();

struct Reporter;
struct TestNamespace {
  std::string_view name;
  std::vector<Test> tests;
  std::vector<TestNamespace> children;

  class iterator {
    struct single_iterator {
      std::vector<Test>::const_iterator it;
      std::vector<Test>::const_iterator end;

      bool operator==(single_iterator const& other) const {
        return it == other.it && end == other.end;
      }
    };

    single_iterator current;
    std::deque<single_iterator> elements;

    void flatten(TestNamespace const& current);

  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = Test;
    using difference_type   = std::ptrdiff_t;
    using pointer           = Test const*;
    using reference         = Test const&;

    iterator() = default;
    explicit iterator(TestNamespace const& ns);

    Test const& operator*() const { return *current.it; }
    Test const* operator->() const { return &operator*(); }
    iterator& operator++();
    bool operator==(iterator const& other) const;
  };

  [[nodiscard]] bool is_empty() const { return tests.empty() && children.empty(); }
  [[nodiscard]] iterator begin() const { return iterator{*this}; }
  [[nodiscard]] static iterator end() { return {}; }
  void insert(const Test& test, size_t i = 0);

  [[nodiscard]] std::size_t count() const;
  bool run(Reporter* reporter);

  void filter(std::span<std::string const> parts);
};

struct TestRoot : TestNamespace {
  bool run(Reporter* reporter);
};

TestRoot get_tests();

}  // namespace rsl::testing