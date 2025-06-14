#pragma once
#include <vector>
#include <string_view>
#include <cstddef>
#include <set>

#include <experimental/meta>

#ifndef RETEST_SCAN_GLOBAL_NAMESPACE
#  define RETEST_SCAN_GLOBAL_NAMESPACE 0
#endif

#include <retest/_impl/util.hpp>
#include <retest/annotations.hpp>
#include <retest/fixture.hpp>
#include <retest/test.hpp>


namespace re {
namespace _impl {
template <std::meta::info R>
consteval std::vector<std::meta::info> expand_test() {
  if (is_function(R)) {
    return {reflect_constant(R)};
  } else if (is_variable(R) && !annotations_of(R, ^^annotations::TParams).empty()) {
    std::vector<std::meta::info> instantiations;
    using generator_type = std::vector<std::meta::info> (*)(std::meta::info);

    for (auto annotation : annotations_of(R, ^^annotations::TParams)) {
      auto params    = extract<annotations::TParams>(annotation);
      auto generator = extract<generator_type>(params.generator);
      instantiations.append_range(generator(R));
    }
    return instantiations;
  } else {
    // TODO fail
    std::unreachable();
  }
}
}  // namespace _impl

namespace _impl {
consteval std::vector<TestDef> expand_class(std::meta::info class_r) {
  std::vector<TestDef> tests{};

  for (auto member : members_of(class_r, std::meta::access_context::current())) {
    if (!has_identifier(member) || identifier_of(member)[0] == '_') {
      continue;
    }
    if ((is_function(member) || is_variable(member)) &&
        has_annotation<annotations::TestTag>(member)) {
      tests.emplace_back(make_test(member));
    }
  }
  return tests;
}

template <auto TUTag>
consteval std::vector<TestDef> discover_tests(std::meta::info target_namespace) {
  std::vector<TestDef> tests{};

  for (auto R : members_of(target_namespace, std::meta::access_context::current())) {
    if (!has_identifier(R) || identifier_of(R)[0] == '_') {
      continue;
    }

    if (!(is_function(R) || is_variable(R) || (is_complete_type(R) && is_class_type(R)))) {
      continue;
    }

    if (has_annotation<annotations::TestTag>(R)) {
      if (is_complete_type(R) && is_class_type(R)) {
        tests.append_range(expand_class(R));
      } else {
        tests.emplace_back(make_test(R));
      }
    } else if (has_annotation<annotations::FixtureTag>(R)) {
      enable_fixture(R);
    }
  }
  return tests;
}
}  // namespace _impl

std::set<TestDef>& registry();

template <std::meta::info NS, auto TUTag = [] {}>
bool enable_namespace() {
  constexpr auto tests = define_static_array(_impl::discover_tests<TUTag>(NS));
  for (auto const& test : tests) {
    registry().insert(test);
  }

  return true;
}

template <std::meta::info NS, auto TUTag = [] {}>
bool enable_tests() {
  if constexpr (NS != ^^:: || RETEST_SCAN_GLOBAL_NAMESPACE) {
    enable_namespace<NS, TUTag>();
  }

  constexpr static auto namespaces = define_static_array(_impl::enumerate_namespaces<TUTag>(NS));
  template for (constexpr auto namespace_ : namespaces) {
    enable_namespace<namespace_, TUTag>();
  }
  return true;
}

}  // namespace re