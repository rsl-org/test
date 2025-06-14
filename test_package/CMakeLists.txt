cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 26)
project(PackageTest CXX)

find_package(retest CONFIG REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example re::retest)

target_compile_options(example PUBLIC 
  "-stdlib=libc++"
  "-freflection"
  "-fannotation-attributes"
  "-fparameter-reflection"
  "-fexpansion-statements"
  "-Wno-c++26-extensions"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)
target_link_options(example PUBLIC "-stdlib=libc++" "-lc++abi")