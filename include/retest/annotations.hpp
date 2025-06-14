#pragma once

#include <experimental/meta>
#include <tuple>
#include <vector>

#include <retest/_impl/util.hpp>

namespace re {
namespace _impl {
template <typename T, std::meta::info Set>
T param_set_to_tuple() {
  constexpr static auto args = [:Set:];
  return []<std::size_t... Idx>(std::index_sequence<Idx...>) {
    return T{[:args[Idx]:]...};
  }(std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <typename T, std::meta::info Sets>
std::vector<T> param_generator() {
  std::vector<T> ret;
  template for (constexpr auto set : [:Sets:]) {
    ret.push_back(param_set_to_tuple<T, set>());
  }
  return ret;
}

struct TestInstance {
  char const* name;
  std::meta::info fnc;
};

consteval std::string stringify_targs(std::span<std::meta::info const> args) {
  std::string result = "<";
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      result += ", ";
    }
    result += display_string_of(args[i]);
  }
  result += ">";
  return result;
}

template <std::meta::info Sets>
constexpr std::vector<TestInstance> tparam_generator(std::meta::info fnc) {
  std::vector<TestInstance> ret;
  auto base_name = identifier_of(fnc);
  template for (constexpr auto set : [:Sets:]) {
    auto name = base_name + stringify_targs([:set:]);
    auto instance = substitute(get_operator(fnc, std::meta::operators::op_parentheses), [:set:]);
    ret.emplace_back(define_static_string(name), instance);
  }
  return ret;
}

template <typename... Ts>
consteval std::meta::info reflect_tuple(std::tuple<Ts...> const& tup, bool reflect_reflections = true) {
  std::array<std::meta::info, sizeof...(Ts)> reflected_element;
  template for (constexpr auto Idx : std::views::iota(0zu, sizeof...(Ts))) {
    if constexpr (std::same_as<Ts...[Idx], std::meta::info>) {
      // effectively disallows std::meta::info CTPs for tests, but allows passing
      // types etc as reflections
      if (!reflect_reflections) {
        reflected_element[Idx] = get<Idx>(tup);
        continue;
      }
    }
    reflected_element[Idx] = std::meta::reflect_constant(get<Idx>(tup));
  }
  return std::meta::reflect_constant_array(reflected_element);
}


}  // namespace _impl

namespace annotations {

struct TestTag {};
constexpr inline TestTag test;

struct ExpectFailureTag {};
constexpr inline ExpectFailureTag expect_failure;

struct FixtureTag {};
constexpr inline FixtureTag fixture;

struct Params {
  std::meta::info generator;

  template <typename... Ts>
  consteval explicit Params(std::vector<std::tuple<Ts...>> const& param_sets) {
    std::vector<std::meta::info> params = {};
    for (auto const& element : param_sets) {
      params.push_back(_impl::reflect_tuple(element));
    }

    auto arg_tuple = substitute(^^std::tuple, {^^Ts...});
    generator =
        substitute(^^_impl::param_generator,
                   {arg_tuple, reflect_constant(std::meta::reflect_constant_array(params))});
  }

  template <typename... Ts>
  consteval explicit(false) Params(std::initializer_list<std::tuple<Ts...>> param_sets)
      : Params(std::vector<std::tuple<Ts...>>{param_sets}) {}

  template <typename... Ts>
  consteval Params(std::vector<std::tuple<Ts...>> (*generator)())
      : generator(std::meta::reflect_constant(generator)) {}
};

struct TParams {
  std::meta::info generator;

  template <typename... Ts>
  consteval explicit TParams(std::vector<std::tuple<Ts...>> const& param_sets) {
    std::vector<std::meta::info> params = {};
    for (auto const& element : param_sets) {
      params.push_back(_impl::reflect_tuple(element, false));
    }

    generator = substitute(^^_impl::tparam_generator,
                           {reflect_constant(std::meta::reflect_constant_array(params))});
  }

  template <typename... Ts>
  consteval explicit(false) TParams(std::initializer_list<std::tuple<Ts...>> param_sets)
      : TParams(std::vector<std::tuple<Ts...>>{param_sets}) {}
};

using params = Params;
using tparams = TParams;

}  // namespace annotations



template <std::meta::info R>
void make_calls() {
  template for (constexpr auto annotation :
                std::define_static_array(annotations_of(R, ^^annotations::Params))) {
    constexpr static annotations::Params params = extract<annotations::Params>(annotation);
    for (auto set : [:params.generator:]()) {
      std::apply([:R:], set);
    }
  }
}

}  // namespace re