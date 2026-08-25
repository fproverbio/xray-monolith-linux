# First xrEngine batch - a research pass (see
# playground/xray-monolith-vulkan-port-notes.md section 21/21a) read every
# #include in all 185 top-level src/xrEngine/*.h|*.cpp files and split them
# into: safe to port now (this file), genuine Vulkan/SDL2 window-bootstrap
# work (a dedicated future pass), and files still blocked on xrGame/xrPhysics/
# xrParticles or on real Vulkan renderer plumbing. This is the first batch,
# not the whole module - most of xrEngine (Engine.cpp/EngineAPI.cpp, the
# scene/level/object graph, the theora/imgui/script glue, etc) is still
# unported and excluded below with per-file reasons, same style as
# xrCDB/xrSound's exclusion comments.
add_library(xrEngine STATIC)

target_include_directories(xrEngine PUBLIC .)

# SDL2/Vulkan window+device bootstrap pass (second xrEngine batch - see
# playground/xray-monolith-vulkan-port-notes.md, section 22): device.cpp/
# Device_create.cpp/Device_destroy.cpp/Device_Initialize.cpp/
# Device_wndproc.cpp/Xr_input.cpp/xr_input_xinput.cpp/MonitorList.cpp/
# IInputReceiver.cpp/imgui_helper.h replace the old Win32
# RegisterClass/CreateWindowEx/WndProc message loop and DirectInput device
# polling with real SDL2 windowing/input and a real Vulkan instance+surface
# (volk-loaded, see Device_Initialize.cpp) - genuinely new code, not a
# mechanical port, adapted from OpenXRay's own SDL2-based x_ray.cpp/
# device.cpp/Xr_input.cpp where a same-shaped reference existed (no
# equivalent for the Vulkan instance/surface half - OpenXRay targets
# OpenGL). xr_input_xinput.h/.cpp are unchanged from upstream: their whole
# body was already commented out (dead XInput/DXUT gamepad code) before
# this pass touched anything, so gamepad support is a documented no-op
# (compiles, reports no controller) rather than a real SDL2 game-controller
# implementation - same "stub what's genuinely out of scope" call as
# xrNetServer's DirectPlay8 removal or xrSound's WASAPI-hotplug removal.
# Physical device selection, logical device and swapchain creation are
# explicitly NOT this pass's job (see the notes file) - only instance +
# window surface.
#
# x_ray.h/xr_input.h/MonitorList.h/device.h are declarations-only from a
# still-earlier pass; xr_input.h/MonitorList.h/device.h now have real
# linked implementations above. x_ray.h's CApplication does not - see the
# x_ray.cpp exclusion note below, its one remaining blocker.
#
# imgui_base_input.cpp, FDemoRecord.cpp: the only 2 of 185 files that
# #include xrGame directly (both just toggle a cursor via
# Game().Level().CurrentEntity()-adjacent calls) - deferred until xrGame
# exists.
#
# tntQAVI.h/.cpp: Video-for-Windows AVI playback, needs a real replacement
# codec, not a mechanical fix - deferred.
#
# GIFResource.h/.cpp: needs giflib vendored first - not landed as of this
# batch, deferred to the next batch once it is.
#
# EngineAPI.cpp, Engine.cpp: bigger, needs the STATIC_RENDERER_R1..4
# decision (notes section 21a point 3) - defer to a later pass. (Engine.h/
# EngineAPI.h themselves are lightweight declarations-only headers, kept
# in scope - they're pulled in transitively by stdafx.h.)
#
# mailSlot.cpp: real Windows named-mailslot IPC (CreateMailslot/
# GetMailslotInfo/GlobalAlloc/WriteFile - a distinct mechanism from POSIX
# message queues, no drop-in portable equivalent) - a genuine Windows API
# with no portable equivalent, not a mechanical fix. Excluded.
#
# Rain.cpp, Environment_misc.cpp, xr_efflensflare.cpp, pure_relcase.cpp,
# XR_IOConsole.cpp, xr_ioc_cmd.cpp: despite being in the original candidate
# list, direct-#include tracing found real dependencies on xrEngine files
# that are themselves large, not-yet-ported, and not part of this batch:
# IGame_Level.h (needs xrCDB/xr_area.h, the CDB<->game-world bridge header
# whose .cpp was already excluded from xrCDB in an earlier module - see
# notes section 17a), IGame_Persistent.h, Render.h's sibling Environment.h
# (weather/lighting, 453 lines), CameraManager.h, Rain.h, bone.h
# (CBoneInstance), and (xr_efflensflare.cpp only) ui_toolscustom.h/
# ui_main.h, which don't exist anywhere in this repo checkout at all.
# Excluded rather than forced - a real dependency on unported core object-
# graph/environment/renderer infrastructure, not a mechanical fix. Revisit
# once IGame_Level.h/Environment.h/CameraManager.h are ported.
# (ObjectDump.cpp turned out fine despite an initial similar-looking
# worry - its only "big" include, xr_object.h, was just a case-mismatch
# path away from already compiling clean, see xr_object.h/IRenderable.h/
# ICollidable.h/Render.h fixes below.)
#
# x_ray.cpp: joins the Rain.cpp/IGame_Level.h exclusion group just above
# for the identical reason, not a Win32/SDL2 gap - its window/Vulkan-
# adjacent pieces (InitSettings/InitConsole/Startup/the WinMain chain)
# were updated in this pass to at least #include cleanly, but
# CApplication::OnEvent (KERNEL:start/disconnect/start_mp_demo handling)
# calls real methods on g_pGameLevel/g_pGamePersistent
# (net_Start/net_Stop/PreStart/Start/Disconnect/OpenDemoFile) - unlike
# every other file in this batch, these aren't bare pointers or TODO'd,
# so the forward declarations device.cpp/Device_destroy.cpp use aren't
# enough here; IGame_Level.h/IGame_Persistent.h being real is a hard
# requirement, not an optimization. WinMain_impl/WinMain themselves are
# also still unguarded Win32 (splash-dialog creation, SEH __try/__except
# around DllMain calls, damn_keys_filter's SystemParametersInfo) -
# secondary to the IGame_Level.h blocker but real, and out of scope for a
# window/device/input bootstrap pass either way. Revisit alongside
# Rain.cpp etc.
target_sources_grouped(
  TARGET xrEngine
  NAME "Engine"
  FILES
  defines.cpp
  stdafx.cpp
  _scripting.cpp
  Device_Misc.cpp
  Device_overdraw.cpp
  CustomHUD.cpp
  edit_actions.cpp
  Envelope.cpp
  EventAPI.cpp
  ICollidable.cpp
  phdebug.cpp
  IRenderable.cpp
  ISheduled.cpp
  LightAnimLibrary.cpp
  ObjectDump.cpp
  perlin.cpp
  pure.cpp
  StatGraph.cpp
  xrHemisphere.cpp
  xrImage_Resampler.cpp
  xrSheduler.cpp
  xrSASH.cpp
  MbHelpers.cpp
  XR_IOConsole_callback.cpp
  line_edit_control.cpp
  line_editor.cpp
)

# SDL2/Vulkan window+device bootstrap - see the file comment above.
target_sources_grouped(
  TARGET xrEngine
  NAME "Device"
  FILES
  device.cpp
  Device_create.cpp
  Device_destroy.cpp
  Device_Initialize.cpp
  Device_wndproc.cpp
  IInputReceiver.cpp
  MonitorList.cpp
  Xr_input.cpp
  xr_input_xinput.cpp
)

# xrXMLParser/xrNetServer are NOT linked here - grepped the final file
# list and neither is #included by anything in this batch (stdafx.h pulls
# in xrCDB/xrSound directly; xml parsing and the network transport aren't
# touched by any of these 26 files). Revisit once a later batch actually
# needs them.
#
# SDL2::SDL2 is PUBLIC: device.h (pulled in transitively by stdafx.h, so
# effectively every xrEngine translation unit) now has a real SDL_Window*
# member and forward-declared Vulkan handle typedefs, so any consumer of
# xrEngine's headers needs <SDL.h> in its include path too - same
# PUBLIC-because-transitively-required reasoning as xrCore/xrCDB/xrSound
# just below. volk::volk/Vulkan::Headers are PRIVATE: only
# Device_Initialize.cpp's implementation calls real Vulkan/volk API -
# device.h itself only forward-declares VkInstance/VkSurfaceKHR as opaque
# pointer types (see device.h's comment), so no downstream consumer needs
# volk.h/vulkan.h just to compile against xrEngine's headers.
target_link_libraries(xrEngine PUBLIC xrCore xrCDB xrSound SDL2::SDL2)
target_link_libraries(xrEngine PRIVATE volk::volk Vulkan::Headers)
