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

# A real, linkable executable. xrRenderStub (Layers/xrAPI) supplies a
# minimal, barebones renderer stand-in - just enough to satisfy
# EngineAPI.cpp's STATIC_RENDERER_R4 symbols (DllMainXrRenderR4,
# SupportsDX11Rendering, dxRenderFactory's Create*/Destroy* family) so the
# link succeeds; it renders nothing. The real DX11 renderer
# (Layers/xrRenderPC_R4) and a real Vulkan backend are both deliberately
# out of scope - see playground/xray-monolith-vulkan-port-notes.md.
#
# Some undefined symbols remain even with the renderer stub in place -
# all pre-existing, already-documented gaps unrelated to rendering: the
# deliberate MP-only exclusion (xrServer::*, game_cl_GameState/
# game_sv_GameState, CBulletManager, demo_info, player_account, ...), the
# xrCDB ISpatial/CObjectSpace exclusion, a handful of xrSound cvars/
# reverb globals, and a couple of standalone CPU-dispatch/text-shaping
# DLL entry points (xrBind_PSGP, ttapi_Done) - none of them touched here.
add_executable(xray-monolith-linux xray_monolith_exe_placeholder.cpp)
# --start-group/--end-group: these static libs reference each other
# circularly (e.g. xrGame <-> xrEngine), and GNU ld's default single-pass
# archive scan can't resolve that regardless of ordering - group them so
# ld keeps re-scanning until nothing new resolves.
target_link_libraries(xray-monolith-linux PRIVATE
  -Wl,--start-group
  xrGame xrEngine xrPhysics xrParticles xrSound xrNetServer xrCDB xrXMLParser xrCore xrRenderStub
  -Wl,--end-group
)
