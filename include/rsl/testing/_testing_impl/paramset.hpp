#pragma once
#include <meta>
#include <vector>
#include <utility>
#include <rsl/span>

namespace rsl::testing::_testing_impl {
struct ParamSet {
  rsl::span<std::meta::info const> value;

  template <typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, ParamSet>)
  consteval explicit(std::invocable<T>) ParamSet(T&& v) : value(make(std::forward<T>(v))) {
    // this constructor is only explicit if the argument is a nullary function
    // this is done so `initializer_list<ParamSet>` does not win over Params' templated generator
    // function ctor
    // TODO only do this if the return type is an iterable with element_type ParamSet or tuple-like
  }

  template <typename... Ts>
    requires(sizeof...(Ts) > 1 && !(std::same_as<std::remove_cvref_t<Ts>, ParamSet> || ...))
  consteval ParamSet(Ts&&... ts) : value(make(std::forward<Ts>(ts)...)) {}

private:
  template <typename... Ts>
  static consteval rsl::span<std::meta::info const> make(Ts&&... ts) {
    std::vector<std::meta::info> elts;
    template for (auto&& elt : {std::forward<Ts>(ts)...}) {
      using type = std::remove_cvref_t<decltype(elt)>;
      if constexpr (std::convertible_to<type, std::string_view>) {
        elts.push_back(std::meta::reflect_constant_string(elt));
      } else if constexpr (std::same_as<type, std::meta::info>) {
        elts.push_back(elt);
      } else {
        elts.push_back(std::meta::reflect_constant(elt));
      }
    }
    return std::define_static_array(elts);
  }
};
}  // namespace rsl::testing::_testing_impl