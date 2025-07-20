#pragma once
#include <cstdint>
#include <string_view>
#include <meta>

namespace rsl::testing::_testing_impl {
template <typename T>
consteval bool has_annotation(std::meta::info R) {
  return !annotations_of(R, dealias(^^T)).empty();
}

consteval std::meta::info get_operator(std::meta::info R, std::meta::operators OP) {
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

consteval bool has_parent(std::meta::info R) {
  // HACK remove this once `std::meta::has_parent` is supported in libc++
  return R != ^^::;
}

consteval std::vector<std::string_view> get_fully_qualified_name(std::meta::info R) {
  std::vector<std::string_view> name{identifier_of(R)};
  auto current = R;
  while (has_parent(current)) {
    current = parent_of(current);
    if (!has_identifier(current)) {
      continue;
    }
    name.emplace_back(define_static_string(identifier_of(current)));
  }
  std::ranges::reverse(name);
  return name;
}
}  // namespace rsl::_testing_impl