cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(rsltest CXX)
add_library(rsltest SHARED)
add_library(rsltest_main SHARED)


set_target_properties(rsltest PROPERTIES OUTPUT_NAME rsltest)
target_compile_options(rsltest PUBLIC 
  "-freflection-latest"
  # "-ftime-trace"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
  "-ftemplate-backtrace-limit=0"
)

target_include_directories(rsltest PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(libassert REQUIRED)
target_link_libraries(rsltest PUBLIC libassert::assert)
target_link_libraries(rsltest_main PUBLIC rsltest)

find_package(rsl-util REQUIRED)
find_package(rsl-config REQUIRED)
find_package(rsl-xml REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(rsltest PUBLIC rsl::util)
target_link_libraries(rsltest PUBLIC rsl::config)
target_link_libraries(rsltest_main PUBLIC rsl::xml)
target_link_libraries(rsltest_main PUBLIC nlohmann_json::nlohmann_json)


add_subdirectory(src)

option(BUILD_TESTING "Enable tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (BUILD_TESTING)
  message(STATUS "Building unit tests")
  include(${CMAKE_SOURCE_DIR}/cmake/rsl-test.cmake)
  # target_compile_definitions(rsltest_test PRIVATE RSL_TEST_NAMESPACE=testing)
  add_library(test_dummy test/dummy.cpp)
  target_enable_tests(test_dummy 
    NAMESPACE demo
    SOURCES example/
    # PREFIX test_
    # PREFIX_REQUIRED
  )
  
  # enable_testing()
  # add_test(NAME rsltest_test COMMAND rsltest_example)
endif()
  
add_subdirectory(ext)

install(TARGETS rsltest_main)
install(TARGETS rsltest)
install(DIRECTORY include/ DESTINATION include)
# if (BUILD_EXAMPLES)
#   add_executable(example_test)
#   add_subdirectory(example)
#   target_link_libraries(example_test PRIVATE rsltest)
#   target_link_libraries(example_test PRIVATE rsltest_main)
# endif()