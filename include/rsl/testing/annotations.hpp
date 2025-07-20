#pragma once
#include <vector>
#include <initializer_list>
#include <meta>

#include <rsl/span>
#include <rsl/string_view>
#include <rsl/assert>

#include "paramset.hpp"

namespace rsl::testing {
namespace annotations {

namespace _impl {
template <bool V>
bool constant_predicate() {
  return V;
}
}  // namespace _impl

struct FixtureTag {};

// test kinds
struct TestTag {};
struct PropertyTag : TestTag {};
struct FuzzTag : TestTag {};

// flags
struct ExpectFailureTag {};

struct Skip {
  bool (*value)() = &_impl::constant_predicate<true>;
};

struct SkipIf {
  static consteval Skip operator()(auto condition) {
    // TODO wrap arbitrary conditions
    return Skip();
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

struct Annotations { // consteval-only
  rsl::span<ParamSet const> targets;
  rsl::span<annotations::Params const> params;

  bool expect_failure = false;
  bool (*skip)() = nullptr;  // this is a function to support conditional skipping
  rsl::string_view name;     // custom base name

  bool is_property_test = false;  // expand missing args as fixtures if false,
                                  // otherwise parameterize with domain
  bool is_fuzz_test = false;

  consteval explicit Annotations(std::meta::info fnc) {
    std::vector<ParamSet> tp_sets;
    std::vector<annotations::Params> p;

    for (auto annotation : annotations_of(fnc)) {
      auto type = remove_cvref(type_of(annotation));
      if (type == ^^annotations::TParams) {
        tp_sets.append_range(extract<annotations::TParams>(constant_of(annotation)).value);
      } else if (type == ^^annotations::Params) {
        p.push_back(extract<annotations::Params>(constant_of(annotation)));
      } else if (type == ^^annotations::ExpectFailureTag) {
        constexpr_assert(!expect_failure, "Cannot annotate with expect_failure more than once.");
        expect_failure = true;
      } else if (type == ^^annotations::Skip) {
        // constexpr_assert(skip == nullptr, "Cannot have more than one skip annotation.");
        skip = extract<annotations::Skip>(constant_of(annotation)).value;
      } else if (type == ^^annotations::Rename) {
        constexpr_assert(name.empty(), "Cannot rename more than once.");
        name = extract<annotations::Rename>(constant_of(annotation)).value;
      } else if (type == ^^annotations::PropertyTag) {
        is_property_test = true;
      } else if (type == ^^annotations::FuzzTag) {
        is_fuzz_test = true;
      }
    }

    targets = define_static_array(tp_sets);
    params  = define_static_array(p);

    if (skip == nullptr) {
      skip = &annotations::_impl::constant_predicate<false>;
    }
  }
};

}  // namespace rsl::testing
