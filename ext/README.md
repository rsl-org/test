# Building with coverage

To enable coverage you need to link to the optional target `rsl::test_cov` for whichever component you want to enable coverage for. Note that if you are testing a library, you will likely want to instrument the library rather than the tests.

This feature is currently only supported with the new `CMakeConfigDeps` Conan generator. To enable it, pass `-c tools.cmake.cmakedeps:new=will_break_next` when pulling in rsl-test.


Alternatively you can copy `coverage_hooks.cpp` into your project's source tree and build it alongside your project. Remember to adjust your compiler and linker options by appending `-fsanitize-coverage=pc-table,trace-pc-guard`.