cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

project(rsltest CXX)

##########################
## Primary test library ##
##########################

add_library(rsltest INTERFACE)
set_target_properties(rsltest PROPERTIES OUTPUT_NAME rsltest)

# TODO remove
target_compile_options(rsltest INTERFACE 
  "-freflection-latest"
  # "-ftime-trace"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)

# TODO guard? this must happen for test TUs
include(FetchContent)
FetchContent_Declare(
  libassert
  GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
  GIT_TAG        v2.2.1 # <HASH or TAG>
  )
FetchContent_MakeAvailable(libassert)
# TODO add cpptrace explicitly

# TODO use FetchContent for this? Can we assume rsl-util is findable through conan?
find_package(rsl-util REQUIRED)

target_link_libraries(rsltest INTERFACE rsl::util)
target_link_libraries(rsltest INTERFACE libassert::assert)

file(GLOB_RECURSE RSL_TEST_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/**")

target_sources(rsltest INTERFACE FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
  FILES ${RSL_TEST_HEADERS})

install(TARGETS rsltest
  EXPORT rsltest-targets
  FILE_SET HEADERS DESTINATION include)


##########################
##     Test runner      ##
##########################

add_library(rsltest_main SHARED)
find_package(rsl-config REQUIRED)
find_package(rsl-xml REQUIRED)
find_package(nlohmann_json REQUIRED)
target_link_libraries(rsltest_main PUBLIC rsltest)
target_link_libraries(rsltest_main PUBLIC rsl::config)
target_link_libraries(rsltest_main PUBLIC rsl::xml)
target_link_libraries(rsltest_main PUBLIC nlohmann_json::nlohmann_json)
add_subdirectory(src)

install(TARGETS rsltest_main)


##########################
##     Unit tests       ##
##########################

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
  

##########################
##    Coverage hooks    ##
##########################
# TODO
# add_subdirectory(ext)

# if (BUILD_EXAMPLES)
#   add_executable(example_test)
#   add_subdirectory(example)
#   target_link_libraries(example_test PRIVATE rsltest)
#   target_link_libraries(example_test PRIVATE rsltest_main)
# endif()