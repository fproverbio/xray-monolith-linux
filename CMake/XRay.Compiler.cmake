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
# DEBUG (bare, no underscore) is the upstream engine's own convention -
# used pervasively across xrCore/xrEngine/xrGame/every render tier (685+
# call sites), including behavior-affecting cases like stdafx.h toggling
# LUABIND_NO_EXCEPTIONS off of it. This CMake port previously only ever
# defined MSVC's _DEBUG/NDEBUG and never bare DEBUG, silently disabling
# every one of those upstream #ifdef DEBUG blocks regardless of build
# type. Defining it alongside _DEBUG restores upstream parity instead of
# touching hundreds of call sites - keeping this fork's divergence from
# upstream as small as possible.
add_compile_definitions(
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Debug>:DEBUG>
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

# The LZO decompressor (src/xrCore/rt_lzo1x_d.ch, upstream miniLZO source,
# unmodified) type-puns through mismatched pointer types in its fast-path
# byte-copy macros (e.g. COPY4: `*(lzo_uint32p)(dst) = *(const
# lzo_uint32p)(src)` on buffers actually typed lzo_bytep). That's UB under
# strict aliasing, so -fno-strict-aliasing is kept as defensive hardening -
# but it turned out NOT to be the cause of the real corruption bug below
# (confirmed by bisection: identical corruption with and without this flag).
# Given how much of this 2010s Windows-era codebase leans on similar
# type-punning, keep it global rather than scoped to LZO.
add_compile_options(-fno-strict-aliasing)

# Real root cause of that corruption, found by bisecting GCC optimizer
# flags against an isolated repro (decompress the same compressed bytes
# with the engine's own lzo1x_decompress vs. the system's independent
# liblzo2, byte-diff the output): GCC's tree loop-vectorizer (part of
# -O3) auto-vectorizes rt_lzo1x_d.ch's fast-path match-copy loop
#   do { COPY4(op, m_pos); op += 4; m_pos += 4; t -= 4; } while (t >= 4);
# LZ77 back-reference copies are allowed to have op/m_pos overlap when the
# match distance is shorter than the match length (e.g. "abcabcabc" is
# encoded as literal "abc" + a length-6 copy from distance 3) - the
# overlap is exactly what lets a short repeated pattern expand, and the
# scalar loop above works because each 4-byte COPY4 sees the bytes the
# *previous* iteration just wrote. GCC's vectorizer doesn't prove
# op/m_pos are non-overlapping (they aren't, by design) but versions the
# loop as if a vector-width chunk copy were equivalent to sequential
# 4-byte steps; for overlapping source/dest that's wrong, silently
# replacing some of the copied bytes with zero. Reproduced deterministically
# with db/configs/configs.db0's squad_descr_agroprom.ltx: the repeated
# substring "ork" (from "snork_weak2", back-referenced via a short-distance
# LZ77 match) decompressed to "\0\0\0" on its 2nd/3rd occurrences, while
# the first (literal, non-back-referenced) occurrence was fine - "cave_sn"
# got truncated to "cave_sn\0\0\0_agr..." and tripped the ini parser's
# "Bad ini section found" assert. -fno-tree-loop-vectorize disables just
# the offending loop vectorizer (confirmed via bisection against
# -fno-tree-slp-vectorize, which does NOT fix it); scoped globally since
# the LZO source here is unmodified upstream miniLZO and other manual
# byte-copy loops in this codebase could hit the same overlap-unsafe
# vectorization.
add_compile_options(-fno-tree-loop-vectorize)

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
