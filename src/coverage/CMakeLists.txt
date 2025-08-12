target_sources(rsltest_cov PUBLIC
  hooks.cpp
  runner.cpp
)

set(COVERAGE_FLAGS 
  -fsanitize-coverage=pc-table,trace-pc-guard
)

target_compile_options(rsltest_cov INTERFACE ${COVERAGE_FLAGS} "-O0" "-g")
target_link_options(rsltest_cov INTERFACE ${COVERAGE_FLAGS})