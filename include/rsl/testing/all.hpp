#pragma once
#include <vector>

#include <libassert/assert.hpp>

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/discovery.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/util.hpp>

namespace rsl {
namespace testing {
bool run(std::vector<Test> const& tests, Reporter& reporter);
}

using testing::annotations::fixture;
using testing::annotations::test;
using testing::annotations::expect_failure;
using testing::annotations::params;
using testing::annotations::tparams;

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