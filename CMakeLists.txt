cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(retest CXX)
add_library(retest)


target_compile_options(retest PUBLIC 
  "-stdlib=libc++"
  "-freflection"
  "-fannotation-attributes"
  "-fparameter-reflection"
  "-fexpansion-statements"
  "-Wno-c++26-extensions"
  "-ftime-trace"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)
target_link_options(retest PUBLIC "-stdlib=libc++" "-lc++abi")
target_include_directories(retest PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(libassert REQUIRED)
target_link_libraries(retest libassert::assert)

# find_package(rsl REQUIRED)
# target_link_libraries(retest rsl::rsl)

add_subdirectory(src)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
install(TARGETS retest)

option(BUILD_TESTING "Enable tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (BUILD_TESTING)
  message(STATUS "Building unit tests")

  add_executable(retest_test "")
  add_subdirectory(test)
  target_link_libraries(retest_test PRIVATE retest)
  
  enable_testing()
  add_test(NAME retest_test COMMAND retest_test)

  if(ENABLE_COVERAGE)
    message(STATUS "Instrumenting for coverage")  

    target_compile_options(rsl_test PRIVATE -g -O0 --coverage)
    target_link_libraries(rsl_test PRIVATE --coverage)
  endif()
endif()
