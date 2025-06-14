#pragma once
#include <cstdint>
#include <string_view>
#include <experimental/meta>

namespace re::_impl {
template <typename T>
consteval bool has_annotation(std::meta::info R) {
  return !annotations_of(R, dealias(^^T)).empty();
}

template <auto TUTag>
consteval std::vector<std::meta::info> enumerate_namespaces(std::meta::info ns) {
  std::vector<std::meta::info> ret{};

  for (auto R : members_of(ns, std::meta::access_context::current())) {
    if (!is_namespace(R) || (has_identifier(R) && identifier_of(R) == "std")) {
      // exclude `std` namespace to make scanning faster
      continue;
    }

    ret.push_back(R);
    ret.append_range(enumerate_namespaces<TUTag>(R));
  }
  return ret;
}

consteval std::meta::info get_operator(std::meta::info R, std::meta::operators OP){
  for (auto member : members_of(type_of(R), std::meta::access_context::current())) {
    if (!is_operator_function(member) && !is_operator_function_template(member)) {
      continue;
    }

    if (operator_of(member) == OP) {
      return member;
    }
  }
  return {};
}


constexpr std::uint32_t fnv1a(char const* str, std::size_t size) {
  std::uint32_t hash = 16777619UL;
  for (std::size_t idx = 0; idx < size; ++idx) {
    hash ^= static_cast<unsigned char>(str[idx]);
    hash *= 2166136261UL;
  }
  return hash;
}

constexpr std::uint32_t fnv1a(std::string_view str) {
  return fnv1a(str.begin(), str.size());
}

}  // namespace re::_impl