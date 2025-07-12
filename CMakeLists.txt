cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(rsltest CXX)
add_library(rsltest SHARED)

set_target_properties(rsltest PROPERTIES OUTPUT_NAME rsltest)
target_compile_options(rsltest PUBLIC 
  "-stdlib=libc++"
  "-freflection"
  "-fannotation-attributes"
  "-fparameter-reflection"
  "-fexpansion-statements"
  # "-Wno-c++26-extensions"
  # "-ftime-trace"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)
target_link_options(rsltest PUBLIC "-stdlib=libc++" "-lc++abi")
target_include_directories(rsltest PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(libassert REQUIRED)
target_link_libraries(rsltest PUBLIC libassert::assert)

# find_package(rsl REQUIRED)
# target_link_libraries(rsltest rsl::rsl)

add_library(rsltest_main)

target_link_libraries(rsltest_main PUBLIC rsltest)

find_package(rsl-config REQUIRED)
target_link_libraries(rsltest_main PUBLIC rsl::config)


add_subdirectory(src)

install(TARGETS rsltest_main)
install(TARGETS rsltest)
install(DIRECTORY include/ DESTINATION include)

option(BUILD_TESTING "Enable tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (BUILD_TESTING)
  message(STATUS "Building unit tests")

  add_executable(rsltest_test)
  add_subdirectory(test)
  # target_compile_definitions(rsltest_test PRIVATE RSL_TEST_NAMESPACE=testing)
  target_link_libraries(rsltest_test PRIVATE rsltest)
  target_link_libraries(rsltest_test PRIVATE rsltest_main)
  
  enable_testing()
  add_test(NAME rsltest_test COMMAND rsltest_example)

  if(ENABLE_COVERAGE)
    message(STATUS "Instrumenting for coverage")  

    target_compile_options(rsltest PRIVATE -g -O0 --coverage)
    target_link_libraries(rsltest PRIVATE --coverage)
  endif()
endif()

if (BUILD_EXAMPLES)
  add_executable(example_test)
  add_subdirectory(example)
  target_link_libraries(example_test PRIVATE rsltest)
  target_link_libraries(example_test PRIVATE rsltest_main)
endif()