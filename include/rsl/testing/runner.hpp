#pragma once
#include <functional>
#include <string>
#include <meta>
#include <tuple>
#include <vector>

#include <rsl/assert>
#include <libassert/assert.hpp>  // TODO use repr instead

#include "annotations.hpp"
#include "_impl/fixture.hpp"

namespace rsl::testing {
struct TestResult;
struct Test;

struct TestRun {
  Test const* test;
  std::function<void()> fnc;
  std::string name;

  [[nodiscard]] TestResult run() const;

  template <std::meta::info Def, std::meta::info Target>
  struct Runner {
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
        name += "<";
        bool first = true;
        template for (constexpr auto T : define_static_array(template_arguments_of(Target))) {
          if (first) {
            first = false;
          } else {
            name += ", ";
          }
          // TODO stringify aliases properly
          name += display_string_of(T);
        }
        name += ">";
      }
      name += std::apply(
          [](auto&&... args) {
            std::string result = "(";
            std::size_t i      = 0;
            ((result += (i++ ? ", " : "") + libassert::stringify(args)), ...);
            result += ")";
            return result;
          },
          args);
      return name;
    }

    template <typename... Ts>
    static TestRun bind(Test const* group, std::tuple<Ts...> args) {
      return {group, std::bind_front(run_one<std::tuple<Ts...>>, args), get_name(args)};
    }
  };
};

namespace _impl {
template <std::meta::info R, Annotations A>
struct Expand {
  std::vector<TestRun> runs;
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
    using Runner = TestRun::Runner<R, Target>;

    if constexpr (A.params.size() == 0) {
      // expand fixtures
      runs.push_back(Runner::bind(group, rsl::testing::_testing_impl::evaluate_fixtures<Target>()));
    } else {
      // expand param annotations
      template for (constexpr auto generator : A.params) {
        expand_param_generator<Runner, generator>();
      }
    }
  }

  template <std::meta::info Target>
  void expand_tparams() {
    constexpr_assert(A.targets.size() != 0,
        std::string("template argument parameterization required for ") + display_string_of(R));

    template for (constexpr auto set : A.targets) {
      expand_params<substitute(Target, set.value)>();
    }
  }

  explicit Expand(Test const* group) : group(group){
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
}  // namespace _impl

}  // namespace rsl::testing