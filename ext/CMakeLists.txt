if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_library(rsltest_cov INTERFACE)
  target_sources(rsltest_cov INTERFACE "${CMAKE_CURRENT_LIST_DIR}/coverage_hooks.cpp")
  add_library(rsl::test_cov ALIAS rsltest_cov)

  # set(COVERAGE_FLAGS
  #   -fsanitize-coverage=pc-table,trace-pc-guard
  # )

  # target_compile_options(rsltest_cov INTERFACE ${COVERAGE_FLAGS} "-O0" "-g")
  # target_link_options(rsltest_cov INTERFACE ${COVERAGE_FLAGS})
  install(TARGETS rsltest_cov)
endif()