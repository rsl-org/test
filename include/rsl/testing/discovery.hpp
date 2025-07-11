#pragma once
#include <vector>
#include <string_view>
#include <cstddef>
#include <set>

#include <meta>

#ifndef RSLTEST_SCAN_GLOBAL_NAMESPACE
#  define RSLTEST_SCAN_GLOBAL_NAMESPACE 0
#endif

#include <rsl/testing/annotations.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/_impl/fixture.hpp>
#include <rsl/testing/_impl/util.hpp>

namespace rsl::testing {
namespace _testing_impl {
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

struct TestDiscovery {
  std::vector<TestDef> tests;
  std::meta::access_context ctx = std::meta::access_context::current();

  consteval void handle_member(std::meta::info R) {
    if (!has_identifier(R) || identifier_of(R)[0] == '_') {
      return;
    }

    if (!(is_function(R) || is_variable(R) || (is_complete_type(R) && is_class_type(R)))) {
      return;
    }

    auto annotations = annotations_of(R);
    for (auto annotation : annotations) {
      if (type_of(annotation) == ^^annotations::TestTag) {
        if (is_complete_type(R) && is_class_type(R)) {
          tests.append_range(expand_class(R));
        } else {
          tests.emplace_back(make_test(R));
        }
        break;
      } else if (type_of(annotation) == ^^annotations::FixtureTag) {
        enable_fixture(R);
        break;
      }
    }
  }

  template <auto Tag>
  consteval void walk_namespace(std::meta::info ns) {
    for (auto R : members_of(ns, ctx)) {
      if (is_namespace(R)) {
        walk_namespace<Tag>(R);
        continue;
      }
      handle_member(R);
    }
  }

  template <auto Tag>
  consteval void walk_global(std::meta::info ns) {
    for (auto R : members_of(ns, ctx)) {
      if (!is_namespace(R)) {
        continue;
      }

      if (has_identifier(R) && identifier_of(R) == "std") {
        continue;
      }

      walk_namespace<Tag>(R);
    }
  }

  template <auto Tag>
  static consteval std::vector<TestDef> find_tests(std::meta::info ns) {
    auto discovery = TestDiscovery();
    if (ns == ^^::&&!RSLTEST_SCAN_GLOBAL_NAMESPACE) {
      discovery.walk_global<Tag>(ns);
    } else {
      discovery.walk_namespace<Tag>(ns);
    }
    return discovery.tests;
  }
};
std::set<TestDef>& registry();
}  // namespace _testing_impl


template <std::meta::info NS, auto TUTag = [] {}>
bool enable_tests() {
  constexpr auto tests = define_static_array(_testing_impl::TestDiscovery::find_tests<TUTag>(NS));
  for (auto const& test : tests) {
    _testing_impl::registry().insert(test);
  }
  return true;
}

}  // namespace rsl::testing