#pragma once
#include <vector>
#include <tuple>
#include <ranges>

namespace re {
template <typename... Ts>
constexpr std::vector<std::tuple<Ts...>> cartesian_product(std::vector<Ts> const&... vs) {
  // if any input is empty, the product is empty
  if ((vs.empty() || ...)) {
    return {};
  }

  std::size_t dimensions[] = {vs.size()...};
  std::vector<std::tuple<Ts...>> result;

  auto max_index = (vs.size() * ... * 1);
  result.reserve(max_index);

  for (auto key : std::views::iota(0zu, max_index)) {
    std::array<std::size_t, sizeof...(vs)> subindices = {};
    for (std::size_t idx = 0zu; idx < sizeof...(vs); ++idx) {
      subindices[idx] = key % dimensions[idx];
      key /= dimensions[idx];
    }

    [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      result.push_back(std::tuple{vs[subindices[Idx]]...});
    }(std::make_index_sequence<sizeof...(vs)>());
  }
  return result;
}

template <typename... Ts>
constexpr std::vector<std::tuple<Ts...>> cartesian_product(std::initializer_list<Ts> const&... vs) {
  return cartesian_product(std::vector(vs)...);
}
}  // namespace re