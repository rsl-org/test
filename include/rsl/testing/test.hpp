#pragma once
#include <string>
#include <functional>
#include <meta>
#include <ranges>
#include <iterator>
#include <deque>
#include <set>
#include <algorithm>

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/_impl/fixture.hpp>
#include <rsl/testing/_impl/util.hpp>

#include <libassert/assert.hpp>

namespace rsl::testing {
struct assertion_failure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct TestResult {
  class Test const* test;
  std::string name;
  bool passed;
  std::string error;
  double duration_ms;
};

namespace _testing_impl {
consteval bool has_parent(std::meta::info R) {
  // HACK remove this once `std::meta::has_parent` is supported in libc++
  return R != ^^::;
}

consteval std::vector<std::string_view> get_fully_qualified_name(std::meta::info R) {
  std::vector<std::string_view> name{identifier_of(R)};
  auto current = R;
  while (has_parent(current)) {
    current = parent_of(current);
    if (!has_identifier(current)) {
      continue;
    }
    name.emplace_back(define_static_string(identifier_of(current)));
  }
  std::ranges::reverse(name);
  return name;
}
}  // namespace _testing_impl

class Test {
public:
  struct TestRun {
    std::function<void()> fnc;
    Test const* test;
    std::string name;

    [[nodiscard]] TestResult run() const;

    template <_testing_impl::TestInstance F, std::meta::info R>
    struct Setter {
      using arg_tuple = [:substitute(
                              ^^std::tuple,
                              parameters_of(F.fnc) | std::views::transform(std::meta::type_of)):];

      static std::vector<arg_tuple> expand_parameters() {
        std::vector<arg_tuple> arg_sets;
        if constexpr (_testing_impl::has_annotation<annotations::Params>(R)) {
          // expand params
          template for (constexpr auto annotation :
                        std::define_static_array(annotations_of(R, ^^annotations::Params))) {
            constexpr static annotations::Params params = extract<annotations::Params>(annotation);
            arg_sets.append_range([:params.generator:]());
          }
        } else {
          // expand fixtures
          arg_sets.push_back(_testing_impl::evaluate_fixtures<F.fnc>());
        }
        return arg_sets;
      }

      static void run_one(arg_tuple const& tuple) {
        if constexpr (is_class_member(R)) {
          auto fixture = [:parent_of(R):]();
          std::apply(fixture.[:F.fnc:], tuple);
        } else if constexpr (is_variable(R)) {
          std::apply([:R:].[:F.fnc:], tuple);
        } else {
          std::apply([:F.fnc:], tuple);
        }
      }

      static std::string stringify_args(arg_tuple const& tuple) {
        return std::apply(
            [](auto&&... args) {
              std::string result = "(";
              std::size_t i      = 0;
              ((result += (i++ ? ", " : "") + libassert::stringify(args)), ...);
              result += ")";
              return result;
            },
            tuple);
      }

      static std::vector<TestRun> make(Test const* test) {
        std::vector<TestRun> runs;
        for (auto&& args : expand_parameters()) {
          auto name = F.name + stringify_args(args);
          runs.emplace_back(std::bind_front(run_one, std::forward<decltype(args)>(args)),
                            test,
                            name);
        }
        return runs;
      }
    };
  };
private:
  template <_testing_impl::TestInstance F, std::meta::info R>
  std::vector<TestRun> runner() const {
    return TestRun::Setter<F, R>::make(this);
  }

  using runner_type = std::vector<TestRun> (Test::*)() const;

  consteval static std::vector<runner_type> expand_targs(std::meta::info R) {
    if (is_function(R)) {
      auto instance = _testing_impl::TestInstance{define_static_string(display_string_of(R)), R};
      return {extract<runner_type>(
          substitute(^^runner, {std::meta::reflect_constant(instance), reflect_constant(R)}))};
    } else if (is_variable(R) && _testing_impl::has_annotation<annotations::TParams>(R)) {
      std::vector<_testing_impl::TestInstance> instantiations;
      using generator_type = std::vector<_testing_impl::TestInstance> (*)(std::meta::info);

      for (auto annotation : annotations_of(R, ^^annotations::TParams)) {
        auto params    = extract<annotations::TParams>(annotation);
        auto generator = extract<generator_type>(params.generator);
        instantiations.append_range(generator(R));
      }

      std::vector<runner_type> runners{};
      runners.reserve(instantiations.size());
      for (auto instance : instantiations) {
        runners.push_back(extract<runner_type>(
            substitute(^^runner, {std::meta::reflect_constant(instance), reflect_constant(R)})));
      }
      return runners;
    } else {
      // TODO fail
      std::unreachable();
    }
  }

  std::span<runner_type const> run_fncs;

public:
  std::string_view name;
  std::span<char const* const> full_name;
  bool expect_failure;
  std::source_location sloc;

  Test() = delete;
  consteval explicit Test(std::meta::info test)
      : name{define_static_string(identifier_of(test))}
      , expect_failure{_testing_impl::has_annotation<annotations::ExpectFailureTag>(test)}
      , sloc(source_location_of(test)) {
    run_fncs = define_static_array(expand_targs(test));

    std::vector<char const*> meta_name;
    for (auto part : _testing_impl::get_fully_qualified_name(test)) {
      meta_name.push_back(std::define_static_string(part));
    }
    full_name = define_static_array(meta_name);
  }
  [[nodiscard]] std::vector<TestRun> get_tests() const;
};

using TestDef = Test (*)();

namespace _testing_impl {
template <std::meta::info R>
Test make_test_impl() {
  return Test(R);
}

consteval TestDef make_test(std::meta::info R) {
  return extract<TestDef>(substitute(^^make_test_impl, {reflect_constant(R)}));
}
}  // namespace _testing_impl

struct Reporter;
struct Output;

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

  [[nodiscard]] iterator begin() const { return iterator{*this}; }
  [[nodiscard]] static iterator end() { return {}; }
  void insert(const Test& test, size_t i = 0);
  
  [[nodiscard]] std::size_t count() const;
  bool run(Reporter* reporter);
};

struct TestRoot : TestNamespace {
  bool run(Reporter* reporter);
};

TestRoot get_tests();

}  // namespace rsl::testing