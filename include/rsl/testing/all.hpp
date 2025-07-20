#pragma once
#include <vector>

#include <libassert/assert.hpp>

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/discovery.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/util.hpp>

namespace rsl {
using testing::fixture;
using testing::test;
using testing::expect_failure;
using testing::params;
using testing::tparams;

}  // namespace rsl::testing


#ifndef RSLTEST_IMPL_USED
#  if defined(__GNUC__) || defined(__clang__)
#    define RSLTEST_IMPL_USED [[gnu::used]]
#  else
#    define RSLTEST_IMPL_USED
#  endif
#endif

#define RSLTEST_ENABLE_NS(NS)                                                 \
  namespace {                                                                 \
  RSLTEST_IMPL_USED [[maybe_unused]] static bool const _rsl_testing_enabled = \
      rsl::testing::enable_tests<^^NS, [] {}>();                                       \
  }

#ifndef RSL_TEST_NAMESPACE
#  define RSL_TEST_NAMESPACE ::
#else
// open the namespace to ensure it exists
namespace RSL_TEST_NAMESPACE {}
#endif

#define RSLTEST_ENABLE RSLTEST_ENABLE_NS(RSL_TEST_NAMESPACE)

namespace rsl::testing {
template <typename T, T V>
  requires (!std::is_reference_v<T>)
struct Anchor {
  constexpr static auto value = 
    std::is_object_v<T> ? std::meta::reflect_constant(V) : std::meta::reflect_function(*V);
};

template <std::meta::info V> 
struct Anchor <std::meta::info, V>{ constexpr static auto value = V; };
}

#define RSLTEST_ANCHOR_IMPL(TYPE, VALUE) \
consteval std::meta::info _rsl_test_anchor(rsl::testing::Anchor<TYPE, VALUE> anchor={}) { return anchor.value; }

#define RSLTEST_ANCHOR(...) RSLTEST_ANCHOR_IMPL(std::meta::info, (^^__VA_ARGS__))
#define RSLTEST_ANCHOR_OVERLOAD(TYPE, VALUE) RSLTEST_ANCHOR_IMPL(TYPE, VALUE)