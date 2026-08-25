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
# IGame_Level.h/IGame_Persistent.h/Environment.h/CameraManager.h/bone.h/
# motion.h/Rain.h (notes file section 24, Part A) turned out to be the
# same story as ObjectDump.cpp/xr_object.h/Render.h below: interface/
# forward-declaration-heavy, not the scary concrete-DirectX-dependency
# blockers they looked like from name/line-count alone. All seven ported
# clean with only the by-now-familiar case-sensitive #include fixes
# (iinputreceiver.h -> IInputReceiver.h, ../xrcdb/xr_area.h ->
# ../xrCDB/xr_area.h, a literal ..\xrServerEntities\ backslash path,
# etc) plus one genuinely-missing macro (RGB(), added to xrCore/
# win32_compat.h next to the existing D3DCOLOR_RGBA - same "real,
# well-documented Win32 macro used purely as a color-packing helper"
# category). That unblocked the six files excluded in the previous
# batch for exactly this reason - Rain.cpp, Environment_misc.cpp,
# xr_efflensflare.cpp, pure_relcase.cpp, XR_IOConsole.cpp, xr_ioc_cmd.cpp
# all compile clean now, plus Environment.cpp/IGame_Level.cpp/
# IGame_Persistent.cpp/CameraManager.cpp/CameraBase.cpp/bone.cpp/
# motion.cpp/PS_instance.cpp/GameMtlLib.cpp/Effector.cpp/EffectorPP.cpp
# themselves (see the "World" group below). Real fixes beyond case/path:
# xr_ioc_cmd.cpp's GetClientRect/MapWindowPoints/ClipCursor and
# GetCursorPos/MonitorFromPoint call sites replaced with the same
# SDL_GetWindowSize/SDL_SetWindowGrab substitution Device_create.cpp/
# Device_destroy.cpp already established, plus an SDL_GetDisplayBounds
# scan for the monitor-at-cursor case (xr_MonitorFromDisplayIndex is
# MonitorList.h's existing HMONITOR encoding); Environment_misc.cpp's
# `create_descriptor(id, false)` calls needed `nullptr` instead (GCC
# rejects `false` as a null pointer constant, MSVC accepted it) and its
# forward-declared-but-never-called `get_ref_count(IUnknown*)` needed
# only a forward `struct IUnknown;` (pointer-only use, never
# dereferenced); GameMtlLib.h needed one more case fix
# (../include/xrapi/xrapi.h -> ../Include/xrAPI/xrAPI.h) and win32_
# compat.h gained `strcmpi` (an MSVC alias for the already-present
# stricmp/_stricmp) plus HDC/HFONT/HBRUSH/HBITMAP/HINSTANCE opaque
# void* handles (Text_Console.h's GDI-based CTextConsole - a
# DEDICATED_SERVER-only debug console, DEDICATED_SERVER itself never
# defined in any real config just like _EDITOR/INGAME_EDITOR - still
# needs to parse since x_ray.cpp #includes Text_Console.h
# unconditionally).
#
# x_ray.cpp itself needed real, not just cosmetic, additional work
# beyond the IGame_Level.h/IGame_Persistent.h unblock (this file's
# CApplication::OnEvent/Startup() call real methods on those types, so
# they're now #included for real, not just forward-declared - fixing
# the DEL_INSTANCE(g_pGamePersistent) "cannot convert to DLL_Pure*"
# error that follows from that). It also still had substantial
# genuinely-unguarded Win32-only code with no _WIN32 guard at all, now
# fixed the same way every other unguarded Windows-only block in this
# port has been handled: the whole Discord Game SDK/ICU rich-presence
# block (xr_ToUTF8/DiscordLog/updateDiscordPresence/Init_Discord/
# clearDiscordPresence and their discord::-typed globals) guarded under
# the file's own pre-existing USE_DISCORD_INTEGRATION macro (already
# only defined under _WIN32); logDlgProc/WinMain_impl/
# stack_overflow_exception_filter/WinMain (the splash-dialog window
# proc, single-instance mutex, DPI-awareness dance, and SEH __try/
# __except wrapper - a real Win32 executable entry point top to bottom,
# called by nothing else in this tree) guarded as one unit under
# _WIN32, matching device.cpp's mt_DiscordThread "guard the whole
# function" precedent rather than an artificial portable/non-portable
# split that doesn't actually exist; damn_keys_filter (StickyKeys/
# FilterKeys/ToggleKeys/screensaver suppression via
# SystemParametersInfo) guarded under _WIN32 too, since its one use
# site is inside the now-guarded WinMain_impl; and InitLauncher/
# FreeLauncher (LoadLibrary of a separate xrLauncher.dll not vendored
# anywhere in this checkout) guarded under _WIN32 as genuinely dead
# code even on Windows - doLauncher(), their only caller, has had its
# entire body commented out upstream already. Two small real SDL
# substitutions matching the established Device_create.cpp/
# Device_destroy.cpp idiom: CApplication::OnEvent's PostQuitMessage(0)
# -> pushing a synthetic SDL_QUIT event (device.cpp's message_loop()
# already breaks on one), and Startup()'s DestroyWindow(logoWindow) ->
# guarded under _WIN32 (logoWindow is only ever set by the
# now-_WIN32-guarded splash dialog, so it's always NULL here on Linux -
# nothing to destroy). HINSTANCE joined HWND/HMONITOR/HMODULE as an
# opaque void* in win32_compat.h.
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

# Game-world/environment cluster unblocked this pass (notes file section
# 24, Part A) - see the file comment above the "Engine" group for the
# full story. x_ray.cpp is here too (the WinMain/CApplication entry
# point) since it only compiles now that IGame_Level.h/IGame_Persistent.h
# are real.
target_sources_grouped(
  TARGET xrEngine
  NAME "World"
  FILES
  IGame_Level.cpp
  IGame_Persistent.cpp
  Environment.cpp
  Environment_misc.cpp
  CameraManager.cpp
  CameraBase.cpp
  Effector.cpp
  EffectorPP.cpp
  bone.cpp
  motion.cpp
  Rain.cpp
  xr_efflensflare.cpp
  pure_relcase.cpp
  PS_instance.cpp
  GameMtlLib.cpp
  XR_IOConsole.cpp
  xr_ioc_cmd.cpp
  x_ray.cpp
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
