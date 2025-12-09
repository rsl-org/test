macro(_impl_fix_bool VARIABLE)
  if(${VARIABLE} MATCHES "(.*-NOTFOUND)|(IGNORE)|(NO)|(OFF)|0|(FALSE)" OR ${VARIABLE} STREQUAL "N" OR ${VARIABLE} STREQUAL "")
    set(${VARIABLE} "false")
  else()
    set(${VARIABLE} "true")
  endif()
endmacro()

function(make_absolute_paths input_var)
  set(input_list "${${input_var}}")
  set(abs_list)

  foreach(p IN LISTS input_list)
    get_filename_component(abs "${p}" REALPATH)
    list(APPEND abs_list "${abs}")
  endforeach()

  set(${input_var} "${abs_list}" PARENT_SCOPE)
endfunction()

function(remove_empty_elements INPUT_LIST OUTPUT_LIST)
    set(RESULT "")
    foreach(ELEM ${${INPUT_LIST}})
        if(NOT ELEM STREQUAL "")
            list(APPEND RESULT ${ELEM})
        endif()
    endforeach()
    set(${OUTPUT_LIST} "${RESULT}" PARENT_SCOPE)
endfunction()

function(to_json_list _list _out)
  set(_list_tmp "${${_list}}")

  list(TRANSFORM _list_tmp APPEND "\"")
  list(TRANSFORM _list_tmp PREPEND "\"")
  list(JOIN _list_tmp ", " _test_sources)
  set(${_out} "[${_test_sources}]" PARENT_SCOPE)
endfunction()

macro(_impl_property VARIABLE QUERY)
  cmake_parse_arguments(
    _PROP_ARG
    "" # flags
    "TARGET" # single value options
    "OR" # multi value options
    ${ARGN}
  )
  if(NOT _PROP_ARG_TARGET)
    set(_PROP_ARG_TARGET ${_TEST_ARG_TARGET})
  endif()

  get_target_property(${VARIABLE} ${_PROP_ARG_TARGET} ${QUERY})
  if(_PROP_ARG_OR)
    foreach(_VAL IN LISTS _PROP_ARG_OR)
      if(${VARIABLE} MATCHES ".*-NOTFOUND" OR ${VARIABLE} STREQUAL "")
        set(${VARIABLE} "${_VAL}")
      endif()
    endforeach()
  endif()

  if(${VARIABLE} MATCHES ".*-NOTFOUND" OR ${VARIABLE} STREQUAL "")
    set(${VARIABLE} "")
  endif()
  unset(_PROP_ARG_TARGET)
  unset(_PROP_ARG_OR)
endmacro()

function(evaluate_config_expr CONFIG_EXPR OUTVAR)
    set(RESULT "${CONFIG_EXPR}")
    # Match all $<$<CONFIG:CFG>:VALUE> patterns
    string(REGEX MATCHALL "\\$<\\$<CONFIG:([a-zA-Z0-9_]+)>:([^>]*)>" MATCHES "${CONFIG_EXPR}")

    foreach(MATCH ${MATCHES})
        # Extract CFG and VALUE
        string(REGEX REPLACE "\\$<\\$<CONFIG:([a-zA-Z0-9_]+)>:([^>]*)>" "\\1" CFG "${MATCH}")
        string(REGEX REPLACE "\\$<\\$<CONFIG:([a-zA-Z0-9_]+)>:([^>]*)>" "\\2" VALUE "${MATCH}")

        # Determine active configuration(s)
        if(CMAKE_BUILD_TYPE STREQUAL "${CFG}" OR (DEFINED CMAKE_CONFIGURATION_TYPES AND CMAKE_CONFIGURATION_TYPES MATCHES "${CFG}"))
            string(REPLACE "${MATCH}" "${VALUE}" RESULT "${RESULT}")
        else()
            string(REPLACE "${MATCH}" "" RESULT "${RESULT}")
        endif()
    endforeach()

    set(${OUTVAR} "${RESULT}" PARENT_SCOPE)
endfunction()

function(collect_interface_usage INTERFACE_TARGET OUT_INCLUDES OUT_DEFS OUT_OPTIONS OUT_LIBS)
  # Track visited targets
  if(NOT DEFINED _visited_targets)
    set(_visited_targets "")
  endif()

  if(NOT TARGET ${INTERFACE_TARGET})
    return()
  endif()

  list(FIND _visited_targets ${INTERFACE_TARGET} _already_visited)
  if(NOT _already_visited EQUAL -1)
    # Already visited, skip recursion
    set(${OUT_INCLUDES} "" PARENT_SCOPE)
    set(${OUT_DEFS} "" PARENT_SCOPE)
    set(${OUT_OPTIONS} "" PARENT_SCOPE)
    set(${OUT_LIBS} "" PARENT_SCOPE)
    return()
  endif()
  list(APPEND _visited_targets ${INTERFACE_TARGET})
  set(_visited_targets "${_visited_targets}" PARENT_SCOPE)

  set(includes "")
  set(defs "")
  set(options "")
  set(libs "")

  # Recurse over linked INTERFACE libraries
  get_target_property(linked ${INTERFACE_TARGET} INTERFACE_LINK_LIBRARIES)
  if(linked)
    foreach(dep ${linked})
      evaluate_config_expr(${dep} dep_fixed)
      # set(dep_fixed ${dep})
      # message("${dep} => ${dep_fixed}")
      if(dep_fixed AND TARGET ${dep_fixed})
        # Recursive call, passing the same visited list
        get_target_property(TYPE ${dep_fixed} dep_type)
        collect_interface_usage("${dep_fixed}" dep_includes dep_defs dep_opts dep_libs)
        list(APPEND includes ${dep_includes})
        list(APPEND defs ${dep_defs})
        list(APPEND options ${dep_opts})
        if(dep_type STREQUAL "INTERFACE_LIBRARY")
          list(APPEND libs ${dep_libs})
        else()
          list(APPEND libs "$<TARGET_FILE:${dep_fixed}>")
        endif()
      else()
        # Non-targets (like system libraries) just append
        list(APPEND libs ${dep})
      endif()
    endforeach()
  endif()

  # Append this target's own interface properties
  get_target_property(my_includes ${INTERFACE_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
  if(my_includes)
    list(APPEND includes ${my_includes})
  endif()

  get_target_property(my_defs ${INTERFACE_TARGET} INTERFACE_COMPILE_DEFINITIONS)
  if(my_defs)
    list(APPEND defs ${my_defs})
  endif()

  get_target_property(my_options ${INTERFACE_TARGET} INTERFACE_COMPILE_OPTIONS)
  if(my_options)
    list(APPEND options ${my_options})
  endif()

  # Return results
  set(${OUT_INCLUDES} "${includes}" PARENT_SCOPE)
  set(${OUT_DEFS} "${defs}" PARENT_SCOPE)
  set(${OUT_OPTIONS} "${options}" PARENT_SCOPE)
  set(${OUT_LIBS} "${libs}" PARENT_SCOPE)
endfunction()


function(target_enable_tests _TEST_ARG_TARGET)
  cmake_parse_arguments(
    _TEST_ARG
    "" # flags
    "NAMESPACE" # single value options
    "SOURCES;DEPENDS" # multi value options
    ${ARGN}
  )

  if(NOT TARGET ${_TEST_ARG_TARGET})
    message(FATAL_ERROR "Target ${_TEST_ARG_TARGET} does not exist")
  endif()

  _impl_property(_lang LINKER_LANGUAGE OR CXX)

  set(_compiler "${CMAKE_${_lang}_COMPILER}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" _build_type)

  set(_compile_flags "${CMAKE_${_lang}_FLAGS}")
  if(CMAKE_BUILD_TYPE)
    string(APPEND _compile_flags " ${CMAKE_${_lang}_FLAGS_${_build_type}}")
  endif()
  separate_arguments(_compile_flags NATIVE_COMMAND "${_compile_flags}")

  _impl_property(_std CXX_STANDARD OR "${CMAKE_CXX_STANDARD}")
  _impl_property(_std_ext CXX_EXTENSIONS OR "${CMAKE_CXX_EXTENSIONS}" "ON")
  _impl_fix_bool(_std_ext)

  _impl_property(_target_options COMPILE_OPTIONS)
  _impl_property(_target_defs COMPILE_DEFINITIONS)
  _impl_property(_target_includes INCLUDE_DIRECTORIES)
  _impl_property(_link_opts LINK_OPTIONS)
  _impl_property(_link_libs LINK_LIBRARIES)

  list(APPEND _compile_flags ${_target_options})


  # if (TARGET rsl-test-dependencies)
  #   get_target_property(_dep_target_type rsl-test-dependencies TYPE)
  #   if(NOT _dep_target_type STREQUAL "INTERFACE_LIBRARY")
  #     message("rsl-test-dependencies must be an INTERFACE library (got ${_dep_target_type})")
  #     # TODO if this isn't an interface library, it could be an old-style test => copy sources and deps
  #   endif()

  add_library(_deps_target INTERFACE)
  target_link_libraries(_deps_target INTERFACE ${_TEST_ARG_TARGET})
  target_link_libraries(_deps_target INTERFACE rsltest)

  include(FetchContent)
  FetchContent_Declare(
    libassert
    GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
    GIT_TAG        v2.2.1 # <HASH or TAG>
  )
  FetchContent_MakeAvailable(libassert)
  target_link_libraries(_deps_target INTERFACE libassert::assert)

  # _impl_property(_dep_link_libs INTERFACE_LINK_LIBRARIES TARGET _deps_target)
  #_impl_property(_dep_includes INTERFACE_INCLUDE_DIRECTORIES TARGET _dep_target)
  #_impl_property(_dep_defines INTERFACE_COMPILE_DEFINITIONS TARGET _dep_target)

  collect_interface_usage(_deps_target _transitive_inc _transitive_defs _transitive_opts _transitive_libs)

  list(APPEND _link_libs ${_dep_link_libs})
  list(APPEND _link_libs ${_transitive_libs})
  list(APPEND _target_includes ${_dep_includes})
  list(APPEND _target_includes ${_transitive_inc})
  list(APPEND _target_defs ${_dep_defines})
  list(APPEND _target_defs ${_transitive_defs})
  list(APPEND _compile_flags ${_transitive_opts})
  
  make_absolute_paths(_TEST_ARG_SOURCES)
  
  to_json_list(_link_libs _link_libs)
  to_json_list(_target_includes _target_includes)
  to_json_list(_target_defs _target_defs)
  to_json_list(_compile_flags _compile_flags)
  to_json_list(_link_opts _link_opts)
  to_json_list(_TEST_ARG_SOURCES _test_sources)
  file(GENERATE
      OUTPUT "test-runner.json"
      CONTENT "
  {
    \"target\": \"${_TEST_ARG_TARGET}\",
    \"options\": {
      \"include_dirs\": ${_target_includes},
      \"compile_options\": ${_compile_flags},
      \"compile_definitions\": ${_target_defs},
      \"link_options\": ${_link_opts},
      \"link_libraries\": ${_link_libs}
    },
    \"project\": {
      \"test_path\": ${_test_sources},
      \"build_path\": \"${CMAKE_BINARY_DIR}\",
      \"project_path\": \"${CMAKE_SOURCE_DIR}\",
      \"namespace\": \"${_TEST_ARG_NAMESPACE}\"
    },
    \"configurations\": {
      \"default\": { 
        \"compiler_path\": \"${_compiler}\",
        \"mode\": \"${_lang}\",
        \"standard\": ${_std},
        \"gnu_extensions\": ${_std_ext},
        \"options\": {}
      }
    }
  }")

  add_executable(${_TEST_ARG_TARGET}-test)
  # target_sources(${_TEST_ARG_TARGET}-test PRIVATE "driver/driver.cpp")
  target_link_libraries(${_TEST_ARG_TARGET}-test PUBLIC rsltest)
  target_link_libraries(${_TEST_ARG_TARGET}-test PUBLIC rsltest_main)
  target_link_libraries(${_TEST_ARG_TARGET}-test PUBLIC ${_TEST_ARG_TARGET})
endfunction()
