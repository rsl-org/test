#pragma once
#include <string>
#include <functional>
#include <meta>
#include <vector>
#include <tuple>

#include <rsl/assert>
#include <rsl/repr>
#include <rsl/testing/assert.hpp>

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/result.hpp>

#include "fixture.hpp"

namespace rsl::testing::_testing_impl {
template <std::meta::info R, std::meta::info Target>
struct FuzzRunner {
  static int run(uint8_t const* Data, size_t Size) {
    // TODO
    return 0;
  }

  // mutator must be able to consider domains
  static size_t mutate(uint8_t* Data, size_t Size, size_t MaxSize, unsigned int Seed) {
    // TODO
    return 0;
  }
};

template <typename TC, std::meta::info Def, std::meta::info Target>
struct TestRunner {
  template <typename T>
  static void run_one(T const& tuple) {
    if constexpr (is_class_member(Def)) {
      auto fixture = [:parent_of(Def):]();
      std::apply(fixture.[:Target:], tuple);
    } else if constexpr (is_variable(Def)) {
      std::apply([:Def:].[:Target:], tuple);
    } else {
      std::apply([:Target:], tuple);
    }
  }

  template <typename... Ts>
  static std::string apply_args_to_name(std::string name, std::tuple<Ts...> args) {
    name += std::apply(
        [](auto&&... args) {
          std::string result = "(";
          std::size_t i      = 0;
          ((result += (i++ ? ", " : "") + repr(args)), ...);
          result += ")";
          return result;
        },
        args);
    return name;
  }

  template <typename... Ts>
  static std::string get_name(std::tuple<Ts...> args) {
    std::string name;
    if constexpr (is_variable(Def)) {
      name += identifier_of(Def);
    } else {
      if constexpr (has_template_arguments(Target)) {
        name += identifier_of(Def);
      } else {
        name += identifier_of(Target);
      }
    }

    if constexpr (has_template_arguments(Target)) {
      name += rsl::serializer::stringify_template_args(Target);
    }

    return apply_args_to_name(name, args);
  }

  template <typename... Ts>
  static TC bind(Test const* group, std::tuple<Ts...> args) {
    constexpr auto ann = _testing_impl::Annotations(Def);
    if constexpr (!ann.name.empty()) {
      constexpr auto name = ann.name;
      return {group,
              std::bind_front(run_one<std::tuple<Ts...>>, args),
              apply_args_to_name(std::string(name), args)};
    } else {
      return {group, std::bind_front(run_one<std::tuple<Ts...>>, args), get_name(args)};
    }
  }
};

template <typename TC, std::meta::info R, _testing_impl::Annotations A>
struct Expand {
  std::vector<TC> runs;
  Test const* group;

  template <typename Runner, annotations::Params Generator>
  void expand_param_generator() {
    if constexpr (Generator.runtime) {
      for (auto&& args : [:Generator.value:]()) {
        runs.push_back(Runner::bind(group, args));
      }
    } else {
      template for (constexpr auto set : [:Generator.value:]) {
        constexpr static auto args = set.value;
        // TODO use p2686/p1061 variadic constexpr structured binding instead
        auto arg_tuple = []<std::size_t... Idx>(std::index_sequence<Idx...>) {
          return std::make_tuple([:args[Idx]:]...);
        }(std::make_index_sequence<set.value.size()>());

        runs.push_back(Runner::bind(group, arg_tuple));
      }
    }
  }

  template <std::meta::info Target>
  void expand_params() {
    using runner = TestRunner<TC, R, Target>;

    if constexpr (A.params.size() == 0) {
      // expand fixtures
      runs.push_back(runner::bind(group, rsl::testing::_testing_impl::evaluate_fixtures<Target>()));
    } else {
      // expand param annotations
      template for (constexpr auto generator : A.params) {
        expand_param_generator<runner, generator>();
      }
    }
  }

  template <std::meta::info Target>
  void expand_tparams() {
    constexpr_assert(
        A.targets.size() != 0,
        std::string("template argument parameterization required for ") + display_string_of(R));

    template for (constexpr auto set : A.targets) {
      expand_params<substitute(Target, set.value)>();
    }
  }

  explicit Expand(Test const* group) : group(group) {
    constexpr auto target_count = A.targets.size();
    constexpr auto param_count  = A.params.size();
    if constexpr (is_variable(R)) {
      using T = [:type_of(R):];
      if constexpr (requires { T::operator(); }) {
        // non-template
        expand_params<^^T::operator()>();
      } else {
        // expand tparam annotations
        expand_tparams<^^T::template operator()>();
      }
    } else if constexpr (is_function(R)) {
      // cannot have targs, expand args directly
      expand_params<R>();
    } else if constexpr (is_function_template(R)) {
      // this needs annotations pulled from a surrogate
      // => we cannot currently query annotations of templates
      expand_tparams<R>();
    } else {
      rsl::compile_error(std::string("Invalid reflection type for test expansion: ") +
                         display_string_of(R));
    }
  }
};
}  // namespace rsl::testing::_testing_impl
