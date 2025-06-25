target_sources(rsltest PUBLIC 
    discovery.cpp
    test.cpp
)

target_sources(rsltest_main PUBLIC main.cpp)
target_include_directories(rsltest PRIVATE .)
target_include_directories(rsltest_main PRIVATE .)