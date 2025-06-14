# retest - a **re**flective **test** framework

A modern, reflective C++ unit test framework.

## Usage
### Simple tests
By default, tests need to be in _some_ namespace. This can be an anonymous namespace. 

If you wish to declare tests in the global namespace, define `RETEST_SCAN_GLOBAL_NAMESPACE` _before_ including `retest.hpp` or any other retest headers. Note that this might result in hitting the constexpr step limit - you can override it by compiling with `-fconstexpr-steps=10000000` or higher.

#### Automatic test discovery
To enable automatic discovery, include `<retest.hpp>` as follows:
```cpp
#include <retest.hpp>

namespace foo {
[[=re::test]]
[[=re::expect_failure]]
void always_fails() {
    ASSERT(false, "oh no");
}
}  // namespace foo
```

The `re::test` annotation flags this function as a test. It must return void, failure is signalled by throwing an exception (which is what happens on assertion failure). `re::expect_failure` makes the test fail if no failure exception was thrown.

#### Manual test discovery

Automatic test discovery walks all namespaces starting from the global namespace. Since this needs to happen in every TU, it can make compilation rather slow. It is possible to manually select a namespace to search for tests in.

To do this use `RETEST_ENABLE_NS` as last statement in your test TU.
```cpp
#include <retest/all.hpp>

namespace testing {

[[=re::test]] 
void always_passes() {}

}  // namespace testing

RETEST_ENABLE_NS(testing)
```

Note that including `<retest/all.hpp>` instead of `<retest.hpp>` will not run the automatic test discovery. You can achieve the same effect by defining `RETEST_SKIP` before including `<retest.hpp>`.

### Test parameterization
Tests can be parameterized.
#### Arguments
```cpp
#include <retest.hpp>
#include <tuple>

namespace {
std::vector<std::tuple<char, int>> make_params() {
    return {{'f', 13}, {'e', 14}};
}

[[=re::test]]
[[=re::params({std::tuple{'a', 10}, {'c', 12}})]]
[[=re::params(re::cartesian_product({'a', 'c'}, {10, 15, 20}))]]
[[=re::params(make_params)]]
void test_with_params(char foo, int bar){
    ASSERT(bar > 5);
    ASSERT(foo != 'x');
};
}  // namespace
```

`re::params` accepts vectors or initializer lists of `std::tuple<Args...>` (where `Args` is the test's parameter types). Additionally you can provide a pointer to a function which returns a vector of aforementioned tuple type. Multiple `re::params` annotations will be chained.

#### Template Arguments
```cpp
#include <retest.hpp>
#include <tuple>

namespace {
[[=re::test]]
[[=re::tparams({std::tuple{^^int, 10}, {^^float, 21}})]]
constexpr inline auto tparam_gt_5 = []<typename T, int I>() static {
    ASSERT(I > 5);
};
}  // namespace
```

Unfortunately template reflection is not scheduled for C++26. This is problematic, since it means we cannot retrieve annotations from templates. To work around this, it is possible to use a `static` lambda. The semantics of `tparams` are similar to `params`, however function pointers are not accepted.

### Fixtures
Tests that have no `re::params` annotations can use fixtures instead.

```cpp
#include <retest.hpp>

namespace {
[[=re::fixture]]
int meta_fixture() { return 21; }

[[=re::fixture]]
int fixture(int meta_fixture) { return meta_fixture * 2; }

[[=re::test]]
void test_with_fixture(int fixture){
    ASSERT(fixture > 5);
};
}  // namespace
```

If a parameter name matches the name of a fixture defined in the same TU, this fixture will be called to produce the arguments for the test invocation. The same also applies to fixtures themselves.

