#pragma once
#include <type_traits>
#include <vector>
#include <initializer_list>
#include <meta>

#include <rsl/span>
#include <rsl/string_view>
#include <rsl/assert>

#include "_testing_impl/util.hpp"
#include "_testing_impl/paramset.hpp"

namespace rsl::testing {

using _testing_impl::ParamSet;

namespace annotations {
struct FixtureTag {};

// test kinds
struct TestTag {};
struct PropertyTag : TestTag {};
struct FuzzTag : TestTag {};

// flags
struct ExpectFailureTag {};

struct Skip {
  bool (*value)() = &_testing_impl::constant_predicate<true>;
};


namespace _impl { 
template <typename T, bool T::*condition>
bool wrap_cli_nsdm() {
  return T::get().*condition;
}
}

struct SkipIf {
  static consteval Skip operator()(bool condition) {
    return {extract<bool (*)()>(
      substitute(^^_testing_impl::constant_predicate, {std::meta::reflect_constant(condition)}))};
    }
    
    static consteval Skip operator()(bool (*condition)()) { return {condition}; }
    
    template <typename T>
    requires requires { T::get(); }
    static consteval Skip operator()(bool T::* condition)  {
      return {extract<bool (*)()>(
        substitute(^^_impl::wrap_cli_nsdm, {^^T, std::meta::reflect_constant(condition)}))};
      }
    };
    
    struct Rename {
      rsl::string_view value;

  static consteval Rename operator()(std::string_view new_name) {
    return Rename(define_static_string(new_name));
  }
};

// parameterization
struct TParams {
  rsl::span<ParamSet const> value;

  consteval explicit TParams(std::vector<ParamSet> const& params)
      : value(std::define_static_array(params)) {}

  consteval explicit TParams(std::initializer_list<ParamSet> params)
      : TParams(std::vector(params)) {}

  consteval explicit TParams(std::vector<ParamSet> (&generator)()) : TParams(generator()) {}
};

struct Params {
  std::meta::info value;
  bool runtime;

  consteval explicit Params(std::vector<ParamSet> const& params)
      : value(std::meta::reflect_constant_array(params))
      , runtime(false) {}

  consteval explicit Params(std::initializer_list<ParamSet> params) : Params(std::vector(params)) {}

  consteval explicit Params(std::vector<ParamSet> (&generator)()) : Params(generator()) {}

  template <typename... Ts>
  consteval explicit Params(std::vector<std::tuple<Ts...>> (*generator)())
      : value(std::meta::reflect_constant(generator))
      , runtime(true) {}
};
}  // namespace annotations

constexpr inline annotations::FixtureTag fixture;

constexpr inline annotations::TestTag test;
constexpr inline annotations::PropertyTag property_test;
constexpr inline annotations::FuzzTag fuzz;

constexpr inline annotations::ExpectFailureTag expect_failure;
constexpr inline annotations::Skip skip;
constexpr inline annotations::SkipIf skip_if;
constexpr inline annotations::Rename rename;

using tparams = annotations::TParams;
using params  = annotations::Params;
}  // namespace rsl::testing
