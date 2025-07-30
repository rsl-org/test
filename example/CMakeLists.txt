target_sources(example_test PRIVATE 
    always_passes.cpp 
    tparams.cpp
    params.cpp
    fixtures.cpp
    conditional.cpp
)

add_executable(params_debug params_debug.cpp)
target_link_libraries(params_debug PUBLIC rsltest)