add_library(xrCDB STATIC)

target_include_directories(xrCDB PUBLIC .)

# cl_raypick.cpp intentionally excluded: the original .vcxproj marks it
# <ExcludedFromBuild>true</ExcludedFromBuild> for every real config
# (Release/Verified/ProfiledDX11/Release-AVX), only ever built for Debug.
# It also includes cl_RAPID.h/cl_defs.h, headers that don't exist anywhere
# in this module (a leftover reference to the old RAPID collision system,
# long since replaced by OPCODE) - it wouldn't compile even if included.
#
# ISpatial.cpp, xr_area.cpp, xr_area_query.cpp, xr_area_raypick.cpp
# (2026-08-30): un-excluded - the "xrEngine hasn't been ported yet" rationale
# this exclusion was written under is stale, xrEngine has since been ported
# and all 4 headers these pulled in (xr_object.h, xrLevel.h, PS_instance.h,
# xr_collide_form.h) now exist there. Fixed 3 files' relative includes for
# this port's usual xrengine -> xrEngine directory-case mismatch (a known
# recurring bug class, see xrEngine/CMakeLists.txt's own case-sensitivity
# notes), plus one
# dead d3d9types.h include in xr_area_raypick.cpp (file doesn't exist
# anywhere; the one macro it could have supplied is already portably shimmed
# in xrCore/win32_compat.h). xr_area_query.cpp needed no changes - it has no
# xrEngine dependency at all.
target_sources_grouped(
  TARGET xrCDB
  NAME "CDB"
  FILES
  Frustum.cpp
  ISpatial.cpp
  ISpatial_q_box.cpp
  ISpatial_q_frustum.cpp
  ISpatial_q_ray.cpp
  ISpatial_verify.cpp
  Opcode.cpp
  OPC_AABB.cpp
  OPC_AABBCollider.cpp
  OPC_AABBTree.cpp
  OPC_Collider.cpp
  OPC_Common.cpp
  OPC_Container.cpp
  OPC_Matrix3x3.cpp
  OPC_Matrix4x4.cpp
  OPC_Model.cpp
  OPC_OBB.cpp
  OPC_OBBCollider.cpp
  OPC_OptimizedTree.cpp
  OPC_Plane.cpp
  OPC_PlanesCollider.cpp
  OPC_Point.cpp
  OPC_Ray.cpp
  OPC_RayCollider.cpp
  OPC_SphereCollider.cpp
  OPC_TreeBuilders.cpp
  OPC_TreeCollider.cpp
  OPC_Triangle.cpp
  OPC_VolumeCollider.cpp
  StdAfx.cpp
  xrCDB.cpp
  xrCDB_box.cpp
  xrCDB_Collector.cpp
  xrCDB_frustum.cpp
  xrCDB_ray.cpp
  xrXRC.cpp
  xr_area.cpp
  xr_area_query.cpp
  xr_area_raypick.cpp
)

target_compile_definitions(xrCDB PRIVATE
  XRCDB_EXPORTS
)

# SDL2::SDL2 is PUBLIC: ISpatial.cpp/xr_area.cpp now pull in xrEngine's
# xr_object.h, which drags in device.h needing <SDL.h> - same reasoning as
# xrEngine's own CMakeLists.txt for why this has to be PUBLIC and not
# PRIVATE (any xrCDB header a future translation unit includes could hit
# the same chain). Not linking xrEngine itself: that would be circular
# (xrEngine already links xrCDB) and isn't needed here - these files only
# use xrEngine's header declarations/inlines, and xrEngine's actual .cpp-
# defined symbols get resolved at the final executable's link step, where
# both static libs are already present.
target_link_libraries(xrCDB PUBLIC xrCore SDL2::SDL2)
