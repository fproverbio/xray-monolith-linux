# xrAPI.cpp is real, unmodified upstream source (this fork's actual
# Layers/xrAPI/xrAPI.dll project) - it defines the Render/RenderFactory/
# UIRender/DU/DRender/PGMLib globals that every other module's xrAPI.h
# only ever *declares* extern; it just needed one case-sensitive #include
# fixed (../../Include/xrApi -> xrAPI, the same recurring bug class as
# every other module on this port) plus a portable stdafx.h (xrCore.h
# instead of <windows.h>, matching every other small module here, e.g.
# xrNetServer). This storage is needed regardless of which renderer tier
# backs it, so this target stays even now that Layers/xrRenderPC_R4
# supplies a real renderer.
#
# render_stub.cpp used to live here too: a stand-in supplying no-op
# bodies for dxRenderFactory's ~17 Create*/Destroy* method pairs plus
# DllMainXrRenderR4/SupportsDX11Rendering, just enough to satisfy
# EngineAPI.cpp's STATIC_RENDERER_R4 symbols before a real renderer
# existed. Deleted now that Layers/xrRenderPC_R4 is wired in - its
# dxRenderFactory.cpp defines the real RenderFactoryImpl and every
# Create*/Destroy* pair, and xrRenderPC_R4/xrRender_R4.cpp defines the
# real DllMainXrRenderR4/SupportsDX11Rendering; compiling both would be a
# straight duplicate-symbol link error, not two valid alternatives. See
# playground/xray-monolith-vulkan-port-notes.md for the full writeup.
add_library(xrAPI STATIC)

target_include_directories(xrAPI PUBLIC .)

target_sources_grouped(
  TARGET xrAPI
  NAME "xrAPI"
  FILES
  stdafx.cpp
  xrAPI.cpp
)

target_link_libraries(xrAPI PUBLIC xrCore)
