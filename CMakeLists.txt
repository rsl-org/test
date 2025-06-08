cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(cpptest CXX)
add_library(cpptest)


target_compile_options(cpptest PUBLIC 
  "-stdlib=libc++"
  "-freflection"
  "-fannotation-attributes"
  "-fparameter-reflection"
  "-fexpansion-statements"
  "-Wno-c++26-extensions"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)
target_link_options(cpptest PUBLIC "-stdlib=libc++" "-lc++abi")
target_include_directories(cpptest PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(libassert REQUIRED)
target_link_libraries(cpptest libassert::assert)

add_subdirectory(src)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
install(TARGETS cpptest)

option(BUILD_TESTING "Enable tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (BUILD_TESTING)
  message(STATUS "Building unit tests")

  add_executable(cpptest_test "")
  add_subdirectory(test)
  target_link_libraries(cpptest_test PRIVATE cpptest)
  
  enable_testing()
  add_test(NAME cpptest_test COMMAND cpptest_test)

  if(ENABLE_COVERAGE)
    message(STATUS "Instrumenting for coverage")  

    target_compile_options(rsl_test PRIVATE -g -O0 --coverage)
    target_link_libraries(rsl_test PRIVATE --coverage)
  endif()
endif()
