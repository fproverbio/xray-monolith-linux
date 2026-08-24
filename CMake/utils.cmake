include_guard()

# Adapted from OpenXRay's cmake/utils.cmake (github.com/OpenXRay/xray-16) -
# same helper, kept so module CMakeLists can group sources for IDE display
# without hand-writing target_sources()/source_group() pairs everywhere.
function(target_sources_grouped)
  cmake_parse_arguments(
    PARSED_ARGS
    ""
    "TARGET;NAME;SCOPE"
    "FILES"
    ${ARGN}
  )

  if(NOT PARSED_ARGS_TARGET)
    message(FATAL_ERROR "You must provide a target name")
  endif()

  if(NOT PARSED_ARGS_NAME)
    message(FATAL_ERROR "You must provide a source group name")
  endif()

  if(NOT PARSED_ARGS_SCOPE)
    set(PARSED_ARGS_SCOPE PRIVATE)
  endif()

  target_sources(${PARSED_ARGS_TARGET} ${PARSED_ARGS_SCOPE} ${PARSED_ARGS_FILES})

  source_group(${PARSED_ARGS_NAME} FILES ${PARSED_ARGS_FILES})
endfunction()
