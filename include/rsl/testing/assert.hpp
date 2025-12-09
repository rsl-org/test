#pragma once
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

#include <rsl/source_location>

namespace rsl::testing {

struct assertion_failure : std::exception {
  std::string message;
  rsl::source_location sloc;

  assertion_failure(std::string_view message, rsl::source_location sloc)
      : message(std::string(message))
      , sloc(sloc) {}
};

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

AssertionTracker& assertion_counter() {
  static AssertionTracker counter{};
  return counter;
}

void track_assertion(AssertionInfo info) {
  assertion_counter().assertions.emplace_back(info);
}
}  // namespace _testing_impl
}  // namespace rsl::testing

#ifdef RSL_TEST_UNIT
// #define LIBASSERT_ASSERT_MAIN_BODY(expr,                                                   \
//                                    name,                                                   \
//                                    type,                                                   \
//                                    failaction,                                             \
//                                    decomposer_name,                                        \
//                                    condition_value,                                        \
//                                    pretty_function_arg,                                    \
//                                    ...)                                                    \
//   rsl::testing::_testing_impl::track_assertion({#expr, "", (condition_value)});            \
//   if (LIBASSERT_STRONG_EXPECT(!(condition_value), 0)) {                                    \
//     libassert::ERROR_ASSERTION_FAILURE_IN_CONSTEXPR_CONTEXT();                             \
//     LIBASSERT_BREAKPOINT_IF_DEBUGGING_ON_FAIL();                                           \
//     failaction;                                                                            \
//     LIBASSERT_STATIC_DATA(name, libassert::assert_type::type, #expr, __VA_ARGS__)          \
//     libassert::detail::process_assert_fail(decomposer_name,                                \
//                                            libassert_params LIBASSERT_VA_ARGS(__VA_ARGS__) \
//                                                pretty_function_arg);                       \
//   }
#  define LIBASSERT_BREAK_ON_FAIL
#  include <libassert/assert.hpp>
#endif