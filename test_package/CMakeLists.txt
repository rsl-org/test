cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 26)
project(PackageTest CXX)

add_executable(example src/example.cpp)

find_package(rsl-test CONFIG REQUIRED)
target_link_libraries(example PRIVATE rsl::test rsl::test_main rsl::test_cov)

target_compile_options(example PRIVATE 
  "-stdlib=libc++"
  "-freflection-latest"
  "-Wno-c++26-extensions"
  # "-fconstexpr-steps=10000000" # required to scan the global namespace
)
target_link_options(example PRIVATE "-stdlib=libc++" "-lc++abi")