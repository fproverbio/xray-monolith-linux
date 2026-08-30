# Minimal, barebones stand-in for the real renderer stack. The real DX11
# renderer (Layers/xrRenderPC_R4, ~115 files) and a real Vulkan backend
# are both deliberately out of scope here - this target exists purely so
# the exploratory xray-monolith-linux executable (src/CMakeLists.txt) can
# reach a clean link, verifying the rest of the engine (xrCore through
# xrGame) is genuinely link-complete. See
# playground/xray-monolith-vulkan-port-notes.md for the full writeup.
#
# xrAPI.cpp is real, unmodified upstream source (this fork's actual
# Layers/xrAPI/xrAPI.dll project) - it already defines the Render/
# RenderFactory/UIRender/DU/DRender/PGMLib globals that every other
# module's xrAPI.h only ever *declares* extern; it just needed one
# case-sensitive #include fixed (../../Include/xrApi -> xrAPI, the same
# recurring bug class as every other module on this port) plus a portable
# stdafx.h (xrCore.h instead of <windows.h>, matching every other small
# module here, e.g. xrNetServer). render_stub.cpp is new: a genuine stub,
# not ported from anything, supplying no-op bodies for dxRenderFactory's
# ~17 Create*/Destroy* method pairs plus DllMainXrRenderR4/
# SupportsDX11Rendering - the handful of symbols EngineAPI.cpp calls
# directly under STATIC_RENDERER_R4.
add_library(xrRenderStub STATIC)

target_include_directories(xrRenderStub PUBLIC .)

target_sources_grouped(
  TARGET xrRenderStub
  NAME "RenderStub"
  FILES
  render_stub.cpp
  stdafx.cpp
  xrAPI.cpp
)

target_link_libraries(xrRenderStub PUBLIC xrCore)
