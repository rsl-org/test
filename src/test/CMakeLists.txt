
add_executable(retest-runner main.cpp)

# CLI
# find_package(rsl-config REQUIRED)
# target_link_libraries(retest-runner PUBLIC rsl::config)

# Output
# find_package(rsl-xml REQUIRED)
# target_link_libraries(retest-runner PUBLIC rsl::xml)
# find_package(nlohmann_json REQUIRED)
# target_link_libraries(retest-runner PUBLIC nlohmann_json::nlohmann_json)

set_target_properties(retest-runner PROPERTIES OUTPUT_NAME retest)