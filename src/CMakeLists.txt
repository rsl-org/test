target_sources(rsltest PUBLIC 
    capture.cpp
    test.cpp
)

target_sources(rsltest_main PRIVATE main.cpp)
target_include_directories(rsltest PRIVATE .)
target_include_directories(rsltest_main PRIVATE .)

add_subdirectory(reporters)