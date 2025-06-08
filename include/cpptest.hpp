#pragma once
#include <vector>
#include <experimental/meta>

#include <iostream>

namespace testing {}

namespace cpptest {

inline namespace annotations {
struct TestTag {};
constexpr inline TestTag test;
}  // namespace annotations

namespace _impl {
template <std::meta::info R>
bool test_runner() {
  [:R:]();
  return true;
}
}  // namespace _impl

struct Test {
  bool (*run)();
  char const* name;

  explicit consteval Test(std::meta::info R)
      : run(extract<bool (*)()>(substitute(^^_impl::test_runner, {reflect_constant(R)})))
      , name(display_string_of(R).data()) {}
};

namespace _impl {
consteval bool is_test(std::meta::info R) {
  return !annotations_of(R, ^^annotations::TestTag).empty();
}

template <auto TUTag>
consteval std::vector<Test> discover_tests(std::meta::info target_namespace = ^^::testing) {
  std::vector<Test> tests{};

  for (auto R : members_of(target_namespace, std::meta::access_context::current())) {
    if (!has_identifier(R) || identifier_of(R)[0] == '_') {
      continue;
    }
    if (is_function(R) && is_test(R)) {
      tests.push_back(Test{R});
    }
  }
  return tests;
}

template <auto TUTag>
consteval std::vector<std::meta::info> enumerate_namespaces(std::meta::info ns) {
  std::vector<std::meta::info> ret{};

  for (auto R : members_of(ns, std::meta::access_context::current())) {
    if (!is_namespace(R) || (has_identifier(R) && identifier_of(R) == "std")) {
      // exclude `std` namespace to make scanning faster
      continue;
    }

    ret.push_back(R);
    ret.append_range(enumerate_namespaces<TUTag>(R));
  }
  return ret;
}
}  // namespace _impl

std::vector<Test>& registry();

template <std::meta::info NS, auto TUTag>
bool enable_namespace() {
  constexpr auto tests = define_static_array(_impl::discover_tests<TUTag>(NS));
  for (auto T : tests) {
    registry().push_back(Test{T});
  }
  return true;
}

template <auto TUTag>
bool enable_tests() {
#ifdef CPPTEST_SCAN_GLOBAL_NAMESPACE
  enable_namespace<^^::, TUTag>();
#endif

  constexpr static auto namespaces = define_static_array(_impl::enumerate_namespaces<TUTag>(^^::));
  template for (constexpr auto NS : namespaces) {
    enable_namespace<NS, TUTag>();
  }
  return true;
}

void run_tests();
}  // namespace cpptest

namespace {
[[maybe_unused]] const bool _cppinfo_registration_helper = cpptest::enable_tests<[] {}>();
}