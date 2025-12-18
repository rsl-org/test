target_sources(rsltest_main PUBLIC main.cpp)
target_include_directories(rsltest_main PRIVATE .)

# add_subdirectory(reporters)
add_subdirectory(platform)