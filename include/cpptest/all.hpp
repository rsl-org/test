#pragma once
#include <vector>

#include <libassert/assert.hpp>

#include <cpptest/annotations.hpp>
#include <cpptest/discovery.hpp>
#include <cpptest/reporter.hpp>
#include <cpptest/test.hpp>
#include <cpptest/util.hpp>


namespace cpptest {
bool run(std::vector<Test> const& tests, Reporter& reporter);

using annotations::test;
using annotations::fixture;

using annotations::expect_failure;
using annotations::params;
using annotations::tparams;

}  // namespace cpptest

#ifndef CPPTEST_IMPL_USED
#  if defined(__GNUC__) || defined(__clang__)
#    define CPPTEST_IMPL_USED [[gnu::used]]
#  else
#    define CPPTEST_IMPL_USED
#  endif
#endif

#define CPPTEST_ENABLE_NS(NS)                                                   \
  namespace {                                                                   \
  CPPTEST_IMPL_USED [[maybe_unused]] static bool const _cppinfo_tests_enabled = \
      cpptest::enable_tests<^^NS, [] {}>();                                     \
  }

#define CPPTEST_ENABLE CPPTEST_ENABLE_NS(::)