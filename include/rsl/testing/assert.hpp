#pragma once
#include <cstddef>
#include <vector>
#include <string_view>

namespace rsl::testing {
struct AssertionInfo {
  std::string_view raw;
  std::string_view expanded;
  bool success;
};
namespace _testing_impl {
struct AssertionTracker {
  std::vector<AssertionInfo> assertions;
  std::string test_name;
};

AssertionTracker& assertion_counter();
}  // namespace _testing_impl
}  // namespace rsl::testing

#define LIBASSERT_ASSERT_MAIN_BODY(expr,                                                       \
                                   name,                                                       \
                                   type,                                                       \
                                   failaction,                                                 \
                                   decomposer_name,                                            \
                                   condition_value,                                            \
                                   pretty_function_arg,                                        \
                                   ...)                                                        \
  rsl::testing::_testing_impl::assertion_counter().assertions.emplace_back(#expr,              \
                                                                           "",                 \
                                                                           (condition_value)); \
  if (LIBASSERT_STRONG_EXPECT(!(condition_value), 0)) {                                        \
    libassert::ERROR_ASSERTION_FAILURE_IN_CONSTEXPR_CONTEXT();                                 \
    LIBASSERT_BREAKPOINT_IF_DEBUGGING_ON_FAIL();                                               \
    failaction;                                                                                \
    LIBASSERT_STATIC_DATA(name, libassert::assert_type::type, #expr, __VA_ARGS__)              \
    libassert::detail::process_assert_fail(decomposer_name,                                    \
                                           libassert_params LIBASSERT_VA_ARGS(__VA_ARGS__)     \
                                               pretty_function_arg);                           \
  }
#define LIBASSERT_BREAK_ON_FAIL
#include <libassert/assert.hpp>