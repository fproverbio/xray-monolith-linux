# Vendored Open Dynamics Engine - real physics dependency for xrPhysics
# (a customized, older ODE fork bundled directly in this engine's tree,
# not a stock upstream release - e.g. dxBody still has separate `pos`/`R`
# members rather than the combined `posr` struct later ODE versions use,
# confirmed by diffing against OpenXRay's own vendored copy at
# Externals/ode/). This CMakeLists.txt's *file list* is adapted near-
# verbatim from OpenXRay's Externals/ode/CMakeLists.txt (github.com/
# OpenXRay/xray-16) - the two trees list the exact same files (same
# names, same layout, right down to the contrib/msvc7/ode_default/
# de_padf_integration.cpp oddity), only the file *contents* differ. The
# real MSVC build (contrib/msvc7/ode_default/default.vcxproj,
# ProjectName "ode") compiles the identical set, confirming this is the
# real/complete file list, not a guess.
#
# No TriMesh collision (collision_trimesh_*.cpp) is vendored or compiled
# here - X-Ray Monolith never shipped ODE's own OPCODE-based trimesh
# support; it has its own custom triangle-list collider instead
# (src/xrPhysics/tri-colliderknoopc/), wired directly into dGeomClass at
# runtime rather than through ODE's dCreateTriMesh.
add_library(ode STATIC)

target_include_directories(ode PUBLIC include)
target_include_directories(ode PRIVATE ode/src)

# dSINGLE: single-precision dReal (float, not double) - matches
# xrPhysics/ode_include.h's own `#ifndef dSINGLE #define dSINGLE` guard,
# set PUBLIC here so it's consistent regardless of include order for any
# other future consumer. dNODEBUG only in Release-shaped configs (mirrors
# OpenXRay's own `$<$<CONFIG:Release,ReleaseMasterGold>:dNODEBUG>` - this
# repo only has Debug/Release, see CMake/XRay.Compiler.cmake) - disables
# ODE's internal dIASSERT/dDEBUGCHECK sanity checks in Release, matching
# upstream's own debug/release split. WIN32/MSVC preprocessor defines
# from the real .vcxproj are deliberately not carried over: every
# WIN32-gated block in this source (windows.h for high-res timing in
# timer.cpp/error.cpp/stack.h, a MSVC/MinGW-only infinity workaround in
# config.h) is already correctly guarded and simply compiles out on
# Linux - not a gap needing a fix.
target_compile_definitions(ode PUBLIC
  dSINGLE
  $<$<NOT:$<CONFIG:Debug>>:dNODEBUG>
)

# _cdecl/__cdecl (used on dError/dDebug/dMessage's declarations in
# include/ode/error.h, and once more on a qsort comparator in
# quickstep.cpp) and __forceinline (odemath.h's dNormalize3/dNormalize4
# etc) are raw MSVC keywords - the same families already made portable
# via xrCore.h's own `#define __cdecl`/`#define _cdecl`/`#define
# __forceinline` for every module that includes xrCore.h first. ODE is a
# standalone vendored library that (correctly) never includes xrCore.h,
# so it needs its own no-op definitions here instead. PRIVATE is enough:
# any xrPhysics translation unit that reaches these same ODE headers
# always includes xrCore.h earlier via stdafx.h, which already defines
# all three macros before ode.h is ever reached.
#
# _copysign (odemath.cpp's dNormalize3_slow): MSVC CRT's underscore-
# prefixed alias for the standard C99/C++11 copysign() - same family,
# aliased straight to the real portable function rather than a no-op.
target_compile_definitions(ode PRIVATE
  "_cdecl="
  "__cdecl="
  "__forceinline=inline"
  "_copysign=copysign"
)

# collision_trimesh_internal.h, objects.h, joint.h, mat.h, stack.h,
# Lcp33.h, quickstep.h, step.h: header-only, pulled in by the .cpp files
# below - not separate compilation units, no ExcludedFromBuild entries in
# the real .vcxproj either.
target_sources_grouped(
  TARGET ode
  NAME "ODE"
  FILES
  ode/src/array.cpp
  ode/src/Bounder33.cpp
  ode/src/collision_kernel.cpp
  ode/src/collision_quadtreespace.cpp
  ode/src/collision_space.cpp
  ode/src/collision_std.cpp
  ode/src/collision_transform.cpp
  ode/src/collision_util.cpp
  ode/src/error.cpp
  ode/src/export-dif.cpp
  ode/src/fastdot.c
  ode/src/fastldlt.c
  ode/src/fastlsolve.c
  ode/src/fastltsolve.c
  ode/src/joint.cpp
  ode/src/lcp.cpp
  ode/src/mass.cpp
  ode/src/mat.cpp
  ode/src/matrix.cpp
  ode/src/memory.cpp
  ode/src/misc.cpp
  ode/src/obstack.cpp
  ode/src/ode.cpp
  ode/src/odemath.cpp
  ode/src/quickstep.cpp
  ode/src/rotation.cpp
  ode/src/step.cpp
  ode/src/stepfast.cpp
  ode/src/StepJointInternal.cpp
  ode/src/testing.cpp
  ode/src/timer.cpp
  ode/src/util.cpp
  contrib/msvc7/ode_default/de_padf_integration.cpp
)
