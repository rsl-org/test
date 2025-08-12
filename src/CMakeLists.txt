target_sources(rsltest PUBLIC 
    capture.cpp
    test.cpp
    runner.cpp
)

add_subdirectory(main)
add_subdirectory(coverage)