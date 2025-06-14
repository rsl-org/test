#pragma once
#include <vector>

#include <libassert/assert.hpp>

#include <retest/annotations.hpp>
#include <retest/discovery.hpp>
#include <retest/reporter.hpp>
#include <retest/test.hpp>
#include <retest/util.hpp>


namespace re {
bool run(std::vector<Test> const& tests, Reporter& reporter);

using annotations::test;
using annotations::fixture;

using annotations::expect_failure;
using annotations::params;
using annotations::tparams;

}  // namespace re

#ifndef RETEST_IMPL_USED
#  if defined(__GNUC__) || defined(__clang__)
#    define RETEST_IMPL_USED [[gnu::used]]
#  else
#    define RETEST_IMPL_USED
#  endif
#endif

#define RETEST_ENABLE_NS(NS)                                                   \
  namespace {                                                                   \
  RETEST_IMPL_USED [[maybe_unused]] static bool const _cppinfo_tests_enabled = \
      re::enable_tests<^^NS, [] {}>();                                     \
  }

#define RETEST_ENABLE RETEST_ENABLE_NS(::)