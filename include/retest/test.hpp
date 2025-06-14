#pragma once
#include <string>
#include <functional>
#include <experimental/meta>

#include <retest/annotations.hpp>
#include <retest/fixture.hpp>
#include <retest/reporter.hpp>
#include <retest/_impl/util.hpp>

#include <libassert/assert.hpp>

namespace re {
struct assertion_failure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct TestResult {
  std::string_view name;
  bool passed;
  std::string error;
  double duration_ms;
};

TestResult timed_run(void (*runner)());

struct Reporter {
  virtual ~Reporter()                                             = default;
  virtual void on_start(size_t total)                             = 0;
  virtual void on_test_start(std::string_view name)               = 0;
  virtual void on_test_end(TestResult const& result)              = 0;
  virtual void on_summary(std::vector<TestResult> const& results) = 0;
  [[nodiscard]] virtual bool colorize() const                     = 0;
};

class Test {
  struct TestRun {
    std::function<void()> fnc;
    Test const* test;
    std::string name;

    [[nodiscard]] TestResult run() const;

    template <_impl::TestInstance F, std::meta::info R>
    struct Setter {
      using arg_tuple = [:substitute(
                              ^^std::tuple,
                              parameters_of(F.fnc) | std::views::transform(std::meta::type_of)):];

      static std::vector<arg_tuple> expand_parameters() {
        std::vector<arg_tuple> arg_sets;
        if constexpr (_impl::has_annotation<annotations::Params>(R)) {
          // expand params
          template for (constexpr auto annotation :
                        std::define_static_array(annotations_of(R, ^^annotations::Params))) {
            constexpr static annotations::Params params = extract<annotations::Params>(annotation);
            arg_sets.append_range([:params.generator:]());
          }
        } else {
          // expand fixtures
          arg_sets.push_back(_impl::evaluate_fixtures<F.fnc>());
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

      static std::vector<std::string> dump() {}

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

  template <_impl::TestInstance F, std::meta::info R>
  std::vector<TestRun> runner() const {
    return TestRun::Setter<F, R>::make(this);
  }

  using runner_type = std::vector<TestRun> (Test::*)() const;

  consteval static std::vector<runner_type> expand_targs(std::meta::info R) {
    if (is_function(R)) {
      auto instance = _impl::TestInstance{define_static_string(display_string_of(R)), R};
      return {
          extract<runner_type>(substitute(^^runner, {std::meta::reflect_constant(instance), reflect_constant(R)}))};
    } else if (is_variable(R) && _impl::has_annotation<annotations::TParams>(R)) {
      std::vector<_impl::TestInstance> instantiations;
      using generator_type = std::vector<_impl::TestInstance> (*)(std::meta::info);

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
  bool expect_failure;

  Test() = delete;
  consteval explicit Test(std::meta::info test)
      : name{define_static_string(identifier_of(test))}
      , expect_failure{_impl::has_annotation<annotations::ExpectFailureTag>(test)} {
    run_fncs = define_static_array(expand_targs(test));
  }
  [[nodiscard]] std::vector<TestRun> get_tests() const;
};

using TestDef = Test (*)();

namespace _impl {
template <std::meta::info R>
Test make_test_impl() {
  return Test(R);
}

consteval TestDef make_test(std::meta::info R) {
  return extract<TestDef>(substitute(^^make_test_impl, {reflect_constant(R)}));
}
}  // namespace _impl
}  // namespace re