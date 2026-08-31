# Minimal skeleton: only xrCore, the most foundational module, is wired up
# so far. Other modules (xrCDB, xrPhysics, xrSound, xrEngine, xrGame,
# renderer tiers, third-party libs) get added incrementally as each one is
# ported - see playground/xray-monolith-vulkan-port-notes.md.
add_subdirectory(xrCore)
add_subdirectory(xrCDB)
add_subdirectory(xrXMLParser)
add_subdirectory(xrSound)
add_subdirectory(xrNetServer)
add_subdirectory(xrEngine)
add_subdirectory(xrParticles)
add_subdirectory(xrPhysics)
add_subdirectory(xrGame)
add_subdirectory(Layers/xrAPI)
add_subdirectory(Layers/xrRenderPC_R4)
add_subdirectory(xrCPU_Pipe)

# A real, linkable executable. Layers/xrRenderPC_R4 is the real DX11
# renderer (compiling its own sources plus the shared Layers/xrRender and
# Layers/xrRenderDX10 pools, see that target's own CMakeLists.txt for the
# full writeup) - it supplies real definitions for EngineAPI.cpp's
# STATIC_RENDERER_R4 symbols (DllMainXrRenderR4, SupportsDX11Rendering,
# dxRenderFactory's Create*/Destroy* family). Layers/xrAPI supplies the
# Render/RenderFactory/UIRender/DU/DRender/PGMLib global storage those
# symbols point at. A real Vulkan backend under dxvk-native is still in
# progress - see playground/xray-monolith-vulkan-port-notes.md.
# xrCPU_Pipe supplies xrBind_PSGP/ttapi_Done (Engine.cpp's CPU-dispatch/
# worker-thread-pool entry points) - real, ported code except for its one
# renderer-coupled function (PLC_calc3_x86), which still gets a no-op
# stub treatment (unrelated to the renderer swap above).
#
# Some undefined symbols remain even so - all pre-existing,
# already-documented gaps unrelated to the renderer: the deliberate
# MP-only exclusion (xrServer::*, game_cl_GameState/game_sv_GameState,
# CBulletManager, demo_info, player_account, ...), the xrCDB
# ISpatial/CObjectSpace exclusion, and a handful of xrSound cvars/reverb
# globals - none of them touched here.
add_executable(xray-monolith-linux xray_monolith_exe_placeholder.cpp)
# --start-group/--end-group: these static libs reference each other
# circularly (e.g. xrGame <-> xrEngine), and GNU ld's default single-pass
# archive scan can't resolve that regardless of ordering - group them so
# ld keeps re-scanning until nothing new resolves.
target_link_libraries(xray-monolith-linux PRIVATE
  -Wl,--start-group
  xrGame xrEngine xrPhysics xrParticles xrSound xrNetServer xrCDB xrXMLParser xrCore xrAPI xrRenderPC_R4 xrCPU_Pipe
  -Wl,--end-group
)
