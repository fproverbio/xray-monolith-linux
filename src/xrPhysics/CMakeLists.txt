# Ground truth for what's really compiled: src/xrPhysics/xrPhysics.vcxproj's
# real <ClCompile> list (58 top-level entries + one cross-module file
# below), same "read the .vcxproj, not just the directory listing"
# discipline as every prior module - see notes file section 25.
#
# dcTriListCollider.cpp intentionally excluded: <ExcludedFromBuild>true
# for every real config (Release/VerifiedDX11/Verified/ProfiledNew/
# Release-AVX) in the real .vcxproj, and the file itself says why at the
# top - "Do NOT build this file seperately. It is included in
# dTriList.cpp automatically." (a classic ODE unity-build-via-#include
# trick; dTriList.cpp does `#include "dcTriListCollider.cpp"` directly).
# Compiling it separately would just double-define every
# dcTriListCollider:: member.
#
# ../xrServerEntities/PHNetState.cpp: a real cross-directory source the
# original .vcxproj compiles directly into xrPhysics.lib (also separately
# compiled into xrGame.lib by xrGame.vcxproj - this engine's usual
# "small header+impl pair compiled per static-lib consumer" idiom rather
# than a shared sub-library). SPHNetState/SPHBonesData only depend on
# xrCore (Fvector/Fquaternion) and NET_Packet (xrCore/NET_utils.h,
# already ported) - no xrServerEntities-internal state, safe to compile
# here unmodified.
add_library(xrPhysics STATIC)

target_include_directories(xrPhysics PUBLIC .)

target_compile_definitions(xrPhysics PRIVATE
  XRPHYSICS_EXPORTS
)

target_sources_grouped(
  TARGET xrPhysics
  NAME "Physics"
  FILES
  ../xrServerEntities/PHNetState.cpp
  ActorCameraCollision.cpp
  collisiondamagereceiver.cpp
  console_vars.cpp
  dcylinder/dCylinder.cpp
  debug_output.cpp
  DisablingParams.cpp
  dRayMotions.cpp
  ElevatorState.cpp
  ExtendedGeom.cpp
  Geometry.cpp
  GeometryBits.cpp
  IActivationShape.cpp
  MathUtils.cpp
  MovementBoxDynamicActivate.cpp
  params.cpp
  PHActivationShape.cpp
  PHActorCharacter.cpp
  PHAICharacter.cpp
  PHCapture.cpp
  PHCaptureInit.cpp
  PHCharacter.cpp
  PHCollideValidator.cpp
  PHContactBodyEffector.cpp
  PHDisabling.cpp
  PHDynamicData.cpp
  PHElement.cpp
  PHElementNetState.cpp
  PHFracture.cpp
  PHGeometryOwner.cpp
  PHInterpolation.cpp
  PHIsland.cpp
  PHJoint.cpp
  PHJointDestroyInfo.cpp
  PHMoveStorage.cpp
  PHObject.cpp
  PHShell.cpp
  PHShellActivate.cpp
  PHShellNetState.cpp
  PHShellSplitter.cpp
  PHSimpleCharacter.cpp
  PHSplitedShell.cpp
  PHStaticGeomShell.cpp
  phvalide.cpp
  PHWorld.cpp
  Physics.cpp
  PhysicsExternalCommon.cpp
  PhysicsShell.cpp
  PhysicsShellAnimator.cpp
  physics_scripted.cpp
  ShellHit.cpp
  stdafx.cpp
  tri-colliderknoopc/dSortTriPrimitive.cpp
  tri-colliderknoopc/dTriBox.cpp
  tri-colliderknoopc/dTriCallideK.cpp
  tri-colliderknoopc/dTriCylinder.cpp
  tri-colliderknoopc/dTriList.cpp
  tri-colliderknoopc/dTriSphere.cpp
  xrPhysics.cpp
)

# Real ProjectReferences in the original .vcxproj: ode (contrib/msvc7/
# ode_default/default.vcxproj), Layers/xrAPI (header-only, reached via
# quote-include relative paths - no library target needed), xrCDB,
# xrSound. xrEngine itself is deliberately not linked - same "compiles
# into the .a, real symbol resolution deferred to the final-exe-link
# pass" stance as every other module that only reaches xrEngine through
# headers so far (xrCDB, xrSound, xrEngine's own EngineAPI.cpp/Engine.cpp
# - see notes section 24b). SDL2::SDL2 is the one real exception: unlike
# every other xrEngine header this module reaches (bone.h/pure.h/
# defines.h/etc - pure declarations, no external include-path
# requirement), ActorCameraCollision.cpp's ../xrEngine/CameraBase.h pulls
# in device.h, which has a genuine compile-time (not just link-time)
# #include <SDL.h> - needs SDL2's headers on the include path even
# though nothing here calls an SDL function. PUBLIC to match xrEngine's
# own reasoning for the same header (SDL2::SDL2 is itself header-only
# from this module's perspective, so PUBLIC costs nothing extra).
target_link_libraries(xrPhysics PUBLIC xrCore xrCDB xrSound ode SDL2::SDL2)
