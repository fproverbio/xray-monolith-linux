include_guard()

# Linux-only from the start - this is a fresh CMake tree for the Vulkan/
# Linux port, not a cross-platform tree also serving an MSVC build (see
# playground/xray-monolith-vulkan-port-notes.md section 0 for why the
# repo's existing stale `cmake` branch wasn't used as a base). Structure
# and several flag choices adapted from OpenXRay's cmake/XRay.Compiler.GNULike.cmake
# (github.com/OpenXRay/xray-16), which already solved this for the same
# X-Ray 1.6.02 codebase lineage.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
    message(FATAL_ERROR "Building with a gcc version less than 11.0 is not supported.")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -Wno-unused-command-line-argument
    -Wno-inconsistent-missing-override
  )
else()
  message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Use GCC or Clang.")
endif()

# x86-64 SSE3 baseline. Monolith's live x86-specific code is limited to
# compiler intrinsics (see notes section 5) - no hand-rolled asm survives
# on the x64 build - so this is the only ISA floor we need to set.
if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
  add_compile_options(-mfpmath=sse -msse3)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-Og -g)
endif()

# Preprocessor definitions Monolith's .vcxproj files set globally across
# every module (see e.g. src/xrCore/xrCore.vcxproj). WIN32 is kept
# deliberately for now: large parts of the tree still branch on it and
# ripping it out is real per-module porting work (see notes), not
# something to paper over at the top-level compiler module. This flag
# only gets us "compiles", not "is actually a Linux-native code path" -
# expect real WIN32-gated logic to need per-file attention as modules
# come online.
add_compile_definitions(
  WIN32
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

set(XRAY_ENABLE_WARNINGS
  -Wall
  -Wextra
  -Wno-unknown-pragmas
  -Wno-strict-aliasing
  -Wno-parentheses
  -Wno-unused-label
  -Wno-unused-parameter
  -Wno-switch
  -Wno-trigraphs
  $<$<CXX_COMPILER_ID:GNU>:-Wno-class-memaccess>
)
add_compile_options(${XRAY_ENABLE_WARNINGS})
