#pragma once
#include <cstddef>
#include <meta>

#include "util.hpp"

namespace rsl::testing::_testing_impl {
template <std::size_t Idx>
struct Fixture {
  friend decltype(auto) evaluate_fixture_impl(Fixture);
};

template <std::meta::info R>
auto evaluate_fixtures() {
  constexpr static auto parameters = std::define_static_array(parameters_of(R));
  return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
    return std::tuple(evaluate_fixture_impl(Fixture<fnv1a(identifier_of(parameters[Idx]))>{})...);
  }(std::make_index_sequence<parameters.size()>());
}

template <std::meta::info R>
struct FixtureEnabler {
  using return_type = [:return_type_of(R):];
  friend decltype(auto) evaluate_fixture_impl(Fixture<fnv1a(identifier_of(R))>) {
    return std::apply([:R:], evaluate_fixtures<R>());
  }
};

consteval void enable_fixture(std::meta::info R) {
  // force `evaluate_fixture` into existence
  (void)is_complete_type(substitute(^^FixtureEnabler, {reflect_constant(R)}));
}
}  // namespace rsl::_testing_impl