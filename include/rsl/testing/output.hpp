#pragma once
#include <cstddef>
#include <string_view>
#include <string>
#include <format>

#include "test.hpp"
#include "_impl/factory.hpp"

namespace rsl::testing {
struct Output {
  virtual ~Output()                        = default;
  virtual void print(std::string_view str) = 0;

  template <typename... T>
  void printf(std::format_string<T...> fmt, T&&... args) {
    print(std::format(fmt, std::forward<T>(args)...));
  }
};

struct Reporter {
  virtual ~Reporter() = default;
  virtual void before_run(TestNamespace const& tests) {}
  virtual void after_run(std::span<TestResult> results) {}

  virtual void before_test_group(Test const& test) {}
  virtual void after_test_group(std::span<TestResult> results) {}

  virtual void before_test(Test::TestRun const& test) = 0;
  virtual void after_test(TestResult const& result)   = 0;

  virtual void list_tests(TestNamespace const& tests);

  virtual void enter_namespace(std::string_view name) {}
  virtual void exit_namespace(std::string_view name) {}

  virtual void finalize(Output& output) {}
};

namespace _reporter_impl {
consteval std::string_view get_name(std::meta::info R) {
  auto annotations = annotations_of(R, ^^annotations::Rename);
  if (annotations.size() == 1) {
    auto opt = extract<annotations::Rename>(constant_of(annotations[0]));
    return opt.value;
  }
  return identifier_of(R);
}
}  // namespace _reporter_impl

using ReporterDef = std::unique_ptr<Reporter> (*)();
std::unordered_map<std::string_view, ReporterDef>& reporter_registry();

template <typename T>
bool register_reporter() {
  reporter_registry()[_reporter_impl::get_name(^^T)] =
      +[] -> std::unique_ptr<Reporter> { return std::make_unique<T>(); };
  return true;
}
}  // namespace rsl::testing

#define REGISTER_REPORTER(classname)                        \
  namespace {                                               \
  [[maybe_unused]] static bool const _reporter_registered = \
      rsl::testing::register_reporter<classname>();         \
  }