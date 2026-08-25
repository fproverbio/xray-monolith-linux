# Genuinely trivial module - 8 top-level .cpp files, all real/compiled in
# every config of the original xrParticles.vcxproj (zero
# ExcludedFromBuild entries). Zero #include of xrEngine/xrGame/Layers/
# xrPhysics anywhere in the module; its only real dependency is xrCore
# plus a single already-solved ../xrCPU_Pipe/ttapi.h include (see notes
# file section 22a - that header needed zero changes when xrEngine first
# consumed it, same story here).
add_library(xrParticles STATIC)

target_include_directories(xrParticles PUBLIC .)

target_sources_grouped(
  TARGET xrParticles
  NAME "Particles"
  FILES
  noise.cpp
  particle_actions.cpp
  particle_actions_collection.cpp
  particle_actions_collection_io.cpp
  particle_core.cpp
  particle_effect.cpp
  particle_manager.cpp
  stdafx.cpp
)

target_compile_definitions(xrParticles PRIVATE
  XR_PARTICLES_EXPORTS
)

target_link_libraries(xrParticles PUBLIC xrCore)
