include_guard()

# Monolith's original MSVC build had 5 configs (Debug/Release/Verified/
# Profiled/ProfiledDX11) plus Release-AVX variants. For the Linux/Vulkan
# port we collapse those to the 3 that are meaningfully different at the
# compiler-flag level; ProfiledDX11 was DX11-profiler-specific and no
# longer applies, AVX is a target-arch toggle, not a distinct config.
set(CMAKE_CONFIGURATION_TYPES
  Debug
  Release
  Verified
)

set(XRAY_DEFAULT_BUILD_TYPE Debug)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (is_multi_config)
  if (NOT CMAKE_DEFAULT_BUILD_TYPE)
    set(CMAKE_DEFAULT_BUILD_TYPE ${XRAY_DEFAULT_BUILD_TYPE})
  endif()
else()
  if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE isn't defined, defaulting to ${XRAY_DEFAULT_BUILD_TYPE}.")
    set(CMAKE_BUILD_TYPE ${XRAY_DEFAULT_BUILD_TYPE})
  endif()
  set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "The type of the build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
endif()
unset(is_multi_config)

# "Verified" mirrors Monolith's USE_VERIFY_IN_RELEASE convention: release
# optimizations with VERIFY()/assertions still compiled in.
set(CMAKE_CXX_FLAGS_VERIFIED ${CMAKE_CXX_FLAGS_RELEASE})
set(CMAKE_C_FLAGS_VERIFIED ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_EXE_LINKER_FLAGS_VERIFIED ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
set(CMAKE_SHARED_LINKER_FLAGS_VERIFIED ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
