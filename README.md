# rsl::test - a reflective test framework

A modern, reflective C++ unit test framework.

## Usage
### Simple tests
By default, tests need to be in _some_ namespace. This can be an anonymous namespace. 

If you wish to declare tests in the global namespace, define `RSLTEST_SCAN_GLOBAL_NAMESPACE` _before_ including `rsltest.hpp` or any other rsltest headers. Note that this might result in hitting the constexpr step limit - you can override it by compiling with `-fconstexpr-steps=10000000` or higher.

#### Automatic test discovery
To enable automatic discovery, include `<rsl/test>` as follows:
```cpp
#include <rsl/test>

namespace foo {
[[=rsl::test]]
[[=rsl::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace foo
```

The `rsl::test` annotation flags this function as a test. It must return void, failure is signalled by throwing an exception (which is what happens on assertion failure). `rsl::expect_failure` makes the test fail if no failure exception was thrown.

#### Manual test discovery

Automatic test discovery walks all namespaces starting from the global namespace. Since this needs to happen in every TU, it can make compilation rather slow. It is possible to manually select a namespace to search for tests in.

To do this use `RSLTEST_ENABLE_NS` as last statement in your test TU.
```cpp
#include <rsl/testing/all.hpp>

namespace testing {

[[=rsl::test]] 
void always_passes() {}

}  // namespace testing

RSLTEST_ENABLE_NS(testing)
```

Note that including `<rsl/testing/all.hpp>` instead of `<rsl/test.hpp>` will not run the automatic test discovery. You can achieve the same effect by defining `RSLTEST_SKIP` before including `<rsl/test.hpp>`.

### Test parameterization
Tests can be parameterized.
#### Arguments
```cpp
#include <rsl/test>
#include <tuple>

namespace {
std::vector<std::tuple<char, int>> make_params() {
    return {{'f', 13}, {'e', 14}};
}

[[=rsl::test]]
[[=rsl::params({std::tuple{'a', 10}, {'c', 12}})]]
[[=rsl::params(rsl::cartesian_product({'a', 'c'}, {10, 15, 20}))]]
[[=rsl::params(make_params)]]
void test_with_params(char foo, int bar){
    ASSERT(bar > 5);
    ASSERT(foo != 'x');
};
}  // namespace
```

`rsl::params` accepts vectors or initializer lists of `std::tuple<Args...>` (where `Args` is the test's parameter types). Additionally you can provide a pointer to a function which returns a vector of aforementioned tuple type. Multiple `rsl::params` annotations will be chained.

#### Template Arguments
```cpp
#include <rsl/test>
#include <tuple>

namespace {
[[=rsl::test]]
[[=rsl::tparams({std::tuple{^^int, 10}, {^^float, 21}})]]
constexpr inline auto tparam_gt_5 = []<typename T, int I>() static {
    ASSERT(I > 5);
};
}  // namespace
```

Unfortunately template reflection is not scheduled for C++26. This is problematic, since it means we cannot retrieve annotations from templates. To work around this, it is possible to use a `static` lambda. The semantics of `tparams` are similar to `params`, however function pointers are not accepted.

### Fixtures
Tests that have no `rsl::params` annotations can use fixtures instead.

```cpp
#include <rsl/test>

namespace {
[[=rsl::fixture]]
int meta_fixture() { return 21; }

[[=rsl::fixture]]
int fixture(int meta_fixture) { return meta_fixture * 2; }

[[=rsl::test]]
void test_with_fixture(int fixture){
    ASSERT(fixture > 5);
};
}  // namespace
```

If a parameter name matches the name of a fixture defined in the same TU, this fixture will be called to produce the arguments for the test invocation. The same also applies to fixtures themselves.

