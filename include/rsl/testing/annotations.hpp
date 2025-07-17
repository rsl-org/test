#pragma once
#include <meta>

#include "_impl/params.hpp"

namespace rsl::testing::annotations {
struct TestTag {};
constexpr inline TestTag test;

struct ExpectFailureTag {};
constexpr inline ExpectFailureTag expect_failure;

struct FixtureTag {};
constexpr inline FixtureTag fixture;

using params  = Params;
using tparams = TParams;

struct Rename {
  char const* value = nullptr;

  static consteval Rename operator()(std::string_view new_name) {
    return Rename(define_static_string(new_name));
  }
};
constexpr inline Rename rename{};

}  // namespace rsl::testing::annotations
