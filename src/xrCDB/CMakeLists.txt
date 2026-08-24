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
# intentionally excluded for this pass: these are the CDB<->game-world
# bridge utilities (CObjectSpace ray/box/frustum queries against live
# CObject/CPS_Instance game entities), not the collision math itself.
# They pull in xrEngine headers (../xrengine/xr_object.h, xrLevel.h,
# PS_Instance.h, xr_collide_form.h) that don't exist yet since xrEngine
# hasn't been ported. The other 4 ISpatial_q_*.cpp files are pure
# spatial-tree walkers with no engine dependency and stay in. Revisit
# this group once xrEngine is scaffolded - see notes file.
target_sources_grouped(
  TARGET xrCDB
  NAME "CDB"
  FILES
  Frustum.cpp
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
)

target_compile_definitions(xrCDB PRIVATE
  XRCDB_EXPORTS
)

target_link_libraries(xrCDB PUBLIC xrCore)
