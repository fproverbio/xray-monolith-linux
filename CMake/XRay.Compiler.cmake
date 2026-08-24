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

# NOT defining WIN32 globally (originally kept as a broad compatibility
# measure - see notes section 14 history). Reverted: a repo-wide grep
# confirms zero code in xrCore actually branches on the project's own
# WIN32 macro (every fix in this port keys off the real compiler-native
# _WIN32/_MSC_VER instead), and keeping it defined actively broke correct
# third-party code - the vendored LZO library's own internal consistency
# check (rt_lzodefs.h) reasonably assumes WIN32 implies LLP64 (32-bit
# unsigned long), which is false on Linux/LP64, and correctly fired its
# own "this should not happen" sanity error once fooled into thinking it
# was targeting real Windows.
add_compile_definitions(
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

# This 2010s-era codebase leans on MSVC's historically lax template-body
# checking (two-phase lookup violations in code paths that happen to never
# get instantiated) and a handful of implicit-conversion patterns GCC
# treats as hard errors by default. -fpermissive downgrades both classes
# to warnings, matching what actually happened under MSVC (silently
# ignored) rather than a new problem introduced by this port - see
# playground/xray-monolith-vulkan-port-notes.md section 14. Worth
# revisiting once the engine builds cleanly end to end, to separate real
# latent bugs from genuinely-dead template code.
add_compile_options(-fpermissive)

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
  $<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-Wno-class-memaccess>
)
add_compile_options(${XRAY_ENABLE_WARNINGS})
