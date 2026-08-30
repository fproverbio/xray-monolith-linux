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
# FDemoRecord.cpp: 1 of the original 2 files that #include xrGame directly
# (just toggles a cursor via a Game().Level().CurrentEntity()-adjacent
# call) - still deferred; MP-diagnostics/demo-recording code, out of scope
# for now regardless of xrGame's availability (see the deliberate MP
# exclusion elsewhere in xrGame/CMakeLists.txt).
#
# tntQAVI.h/.cpp: Video-for-Windows AVI playback, needs a real replacement
# codec, not a mechanical fix - deferred.
#
# imgui_base.cpp, imgui_helper.cpp: Dear ImGui now has a real CMake
# target (sdk/include/imgui, commit a0382f3c) - both compile clean
# against it with zero further changes.
#
# imgui_base_input.cpp (2026-08-30): the other of the original 2 xrGame-
# including files - now wired in, xrGame exists. Needed one real fix
# beyond just resolving its ../xrGame/ includes: ImGui_UpdateKeyboardCodePage()
# used several Win32-only APIs (GetKeyboardLayout/MAKELCID/GetLocaleInfoA)
# to convert a single legacy WM_CHAR UTF-16 code unit via the active
# keyboard layout's ANSI codepage - entirely unnecessary here, since
# Device_wndproc.cpp's SDL_TEXTINPUT handler already receives a whole
# UTF-8 string per event (its own comment already flagged this exact
# follow-up). Switched ide::InputChar() to take that UTF-8 string
# directly and forward it to ImGui's own io.AddInputCharactersUTF8() -
# dropped the codepage lookup entirely rather than porting it, along with
# the now-fully-unused UpdateInputLang()/keyboard_code_page member (no
# other call site referenced either).
#
# EngineAPI.cpp, Engine.cpp: the STATIC_RENDERER_R1..4 decision (notes
# section 21a point 3 / section 24 Part B) is now made - STATIC_RENDERER_R4
# defined below as a PRIVATE compile definition on this target. R4 is the
# DX11 renderer and the modern/default choice: 5 of xrEngine.vcxproj's 13
# real configs select it (ReleaseR4, ReleaseR4-AVX, ProfiledDX11,
# VerifiedDX11, and the bare "Verified" config), more than any other
# renderer number (R1/DX8 and R3/DX10 each get 2, R2/DX9 gets 3 including
# VerifiedDX9). EngineAPI.cpp/Engine.cpp now reference DllMainXrRenderR4/
# xrFactory_Create/xrFactory_Destroy/DllMainXrGame/SupportsDX11Rendering/
# xrBind_PSGP/ttapi_Done - none of which exist anywhere in this tree yet,
# which is fine and expected at the "compiles into libxrEngine.a" stage
# (notes section 21a point 1/3) - a final-exe-link concern for a much
# later milestone, not a blocker here. Two genuinely Windows-only bits
# guarded under _WIN32 rather than stubbed: EngineAPI.cpp's vTune
# (vTuneAPI.dll) LoadLibrary/GetProcAddress calls (a real prebuilt
# native library not vendored anywhere in this checkout, same category
# as the Discord Game SDK/xrLauncher.dll, matching xrCore/xrDebug.cpp's
# identical treatment of its own DBGHELP.DLL LoadLibrary) and the
# SetErrorMode(SEM_FAILCRITICALERRORS) pair around DllMainXrRenderR4's
# call site (a modal-error-dialog suppression with no Linux/X11
# equivalent, moot now that the renderer is called directly rather than
# through a real LoadLibrary that could pop one anyway).
#
# mailSlot.cpp (2026-08-30): un-excluded - its named-mailslot IPC
# (CreateMailslot/GetMailslotInfo/GlobalAlloc/WriteFile) is still a genuine
# Windows API with no portable equivalent, but it only exists to let an
# external process inject console commands into a running DEBUG build -
# not worth a real IPC port, so msCreate()/msRead() (the two functions
# Engine.cpp/EventAPI.cpp actually call) were stubbed to no-ops instead of
# ported; msWrite() (the unused write side of the same mechanism) was
# dropped outright, matching this port's established practice of stubbing
# rather than porting Windows-only debug diagnostics with no real use here
# (see e.g. the SEH/SetErrorMode/g_profiler removals elsewhere).
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
#
# Second-wave mechanical sweep (notes file section 24, Part C): most of
# the remaining candidates turned out to be more case-sensitivity fixes
# (igame_level.h -> IGame_Level.h, render.h -> Render.h, ../xrcdb/ ->
# ../xrCDB/, a literal ..\Include\xrRender\Kinematics.h backslash path,
# etc - the same pattern as every prior batch) plus a few real fixes:
# Feel_Touch.h/Feel_Vision.h (private pure_relcase base classes) needed
# `friend class pure_relcase;` - pure_relcase's templated constructor
# does static_cast<class_type*>(this) to bind its relcase callback, a
# base-to-derived conversion that needs the private-inheritance
# relationship visible from pure_relcase's own code, not just the
# derived class's; MSVC is permissive about this access check across
# template-instantiation boundaries, GCC enforces it strictly (only
# surfaced once each class's constructor actually instantiated
# pure_relcase's constructor with itself as class_type - same "first
# instantiation" pattern as this port's other MSVC-permissive template
# bugs). Feel_Vision.cpp additionally needed IGame_Level.h reordered
# before Feel_Vision.h in its own #include list, for the same
# first-parse-time-non-dependent-name-lookup reason pure_relcase.cpp/
# Feel_Touch.cpp already get right (g_pGameLevel/CObjectList::
# relcase_register must already be declared when pure_relcase.h's
# template is first parsed, not just by the time it's instantiated).
# xr_object.cpp's <intrin.h>/_InterlockedCompareExchange (a real MSVC
# intrinsic, one call site) got a portable win32_compat.h shim via GCC/
# Clang's __sync_val_compare_and_swap builtin (identical CAS semantics).
# GIFResource.cpp (giflib landed in an earlier commit, e1447d9e) had two
# genuine `Memory.mem_alloc(size)` calls using the wrong overload - this
# build defines DEBUG_MEMORY_NAME (memory_monitor.h's USE_MEMORY_MONITOR
# is unconditional), so only the 2-arg `mem_alloc(size, name)` overload
# exists; switched to the existing `xr_malloc(size)` wrapper that already
# handles both branches, rather than hand-writing a name argument.
# fmesh.cpp's `#include <d3dx9.h>` turned out to guard nothing real -
# its one real caller (ConvertVertices, D3DXGetFVFVertexSize) is itself
# entirely commented out upstream; guarded under _WIN32 as dead weight.
# ai_script_lua_debug.cpp/ai_script_lua_extension.cpp: now included (notes
# file section 25). Their blocker was real - ai_script_space.h wants
# `luabind::functor<T>`, which genuinely did not exist anywhere in the
# vendored luabind-deboostified fork (upstream dropped it on purpose, see
# its own doc/changes.txt: "removed functor (use object with
# call_function() instead)") - but it turned out to be a missing-header
# gap, not a missing-library one: src/xrServerEntities/pch_script.h (this
# engine's OWN sibling precompiled header for lua/luabind users) already
# `#include <luabind/functor.hpp>` right after <luabind/object.hpp>, and
# three real call sites elsewhere in the tree (pch_script.h, xrGame's
# GameTask.h, xrGame's Level_input.cpp) already expected that exact path
# to resolve - ai_script_space.h was just missing the same line its
# sibling already had. Fixed by vendoring src/3rd party/luabind/luabind/
# functor.hpp (a `luabind::functor<T>` shim adapted from OpenXRay's
# src/xrScriptEngine/Functor.hpp, built on primitives this fork's own
# luabind kept - adl::object, the object-based call_function() overload,
# default_converter/native_converter_base) and adding the missing
# `#include <luabind/functor.hpp>` to ai_script_space.h itself. Text_Console.cpp/Text_Console_WndProc.cpp stay
# excluded too: real Win32 GDI window creation (RegisterClass/
# CreateWindow/GetDC/BitBlt/...) for the DEDICATED_SERVER-only debug
# console (DEDICATED_SERVER itself dead code, see Text_Console.h's own
# note above) - genuine window-creation work for a code path nothing
# ever actually takes, not worth porting ahead of a real dedicated-
# server pass.
target_sources_grouped(
  TARGET xrEngine
  NAME "Engine"
  FILES
  defines.cpp
  stdafx.cpp
  _scripting.cpp
  ai_script_lua_debug.cpp
  ai_script_lua_extension.cpp
  Engine.cpp
  EngineAPI.cpp
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
  mailSlot.cpp
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
  GameMtlLib_Engine.cpp
  XR_IOConsole.cpp
  XR_IOConsole_control.cpp
  XR_IOConsole_get.cpp
  xr_ioc_cmd.cpp
  x_ray.cpp
  Environment_render.cpp
  thunderbolt.cpp
  FDemoPlay.cpp
  Feel_Touch.cpp
  Feel_Vision.cpp
  GameFont.cpp
  IGame_Level_check_textures.cpp
  IGame_ObjectPool.cpp
  ObjectAnimator.cpp
  Render.cpp
  SkeletonMotions.cpp
  Stats.cpp
  cf_dynamic_mesh.cpp
  fmesh.cpp
  interp.cpp
  xr_collide_form.cpp
  xr_object.cpp
  xr_object_list.cpp
)

# Theora (FMV/cutscene decode, theora landed fb77ca0f) and giflib
# (animated-GIF UI playback, gif landed e1447d9e) - both now included per
# the notes file section 24, Part C.
target_sources_grouped(
  TARGET xrEngine
  NAME "Video"
  FILES
  xrTheora_Stream.cpp
  xrTheora_Surface.cpp
  xrTheora_Surface_mmx.cpp
  GIFResource.cpp
)

# Dear ImGui debug UI overlay (imgui landed a0382f3c) - see the file
# comment above the "Engine" group for the full story.
target_sources_grouped(
  TARGET xrEngine
  NAME "ImGui"
  FILES
  imgui_base.cpp
  imgui_base_input.cpp
  imgui_helper.cpp
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
# xrLuabind is PUBLIC for the same reason SDL2::SDL2 is: ai_script_space.h
# (pulled in by ai_script_lua_extension.h, one of this target's own PUBLIC
# headers - see the "Engine" group above) #includes <lua.hpp> and
# <luabind/{luabind,object,functor}.hpp>, so any downstream consumer of
# xrEngine's headers (a future xrGame, notably) needs those include paths
# too, not just this target's own .cpp files.
target_link_libraries(xrEngine PUBLIC xrCore xrCDB xrSound SDL2::SDL2 xrLuabind)
target_link_libraries(xrEngine PRIVATE volk::volk Vulkan::Headers)

# theora/gif are PRIVATE: xrTheora_Stream.h/xrTheora_Surface.h/
# GIFResource.h's own #includes (<theora/theora.h>, gif_lib.h) are
# reachable via these targets' own PUBLIC include dirs, and no other
# xrEngine header re-exposes their types, so downstream consumers of
# xrEngine's headers don't need either just to compile against it.
target_link_libraries(xrEngine PRIVATE theora gif)

# imgui is PRIVATE: imgui_base.h only forward-declares through the
# already-abstract ../Include/xrRender/ImGuiRender.h interface, no
# xrEngine header re-exposes real Dear ImGui types.
target_link_libraries(xrEngine PRIVATE imgui)

# STATIC_RENDERER_R4 - see EngineAPI.cpp's exclusion-turned-inclusion
# comment above (notes section 24, Part B) for the full reasoning. PRIVATE:
# only EngineAPI.cpp/EngineAPI.h's own #ifdef STATIC_RENDERER_R1..4 blocks
# (and the DLL_MAIN_RENDERER macro they select) ever look at this macro -
# no other xrEngine header branches on it, so it's not part of this
# target's public interface.
target_compile_definitions(xrEngine PRIVATE STATIC_RENDERER_R4)

# ENGINE_BUILD - every real ENGINE_BUILD;STATIC_RENDERER_R2|R4;... config in
# the original xrEngine.vcxproj defines this directly (grep-verified), so
# this matches stock settings, not a new invention. It matters right now
# for ai_script_lua_extension.cpp/.h (notes section 25): stdafx.h's own
# comment above the DLL_API/ENGINE_API block spells out the intent - "you
# must define ENGINE_BUILD [when] building the engine itself and not
# define it if you are about to build DLL" - and ai_script_lua_extension.*
# uses it to gate its xrGame-dependent half (vfExportToLua/
# vfLoadStandardScripts/bfLoadFile/the `#include "ai_space.h"` at the top
# of the .cpp) out of engine builds entirely, leaving only the
# always-available half (bfPrintOutput/vfPrintError/bfLoadBuffer/
# bfLoadFileIntoNamespace/bfGetNamespaceTable/bfIsObjectPresent/
# lua_namespace_table) - exactly the split this pass needs to compile
# these 2 files without pulling in any of xrGame. PRIVATE: DLL_API/
# ENGINE_API both already expand to nothing either way (their real
# __declspec(dllimport/dllexport) lines are commented out in stdafx.h,
# same project-wide Windows-attribute stripping as everywhere else), so
# no other xrEngine header's observable behavior depends on this macro.
target_compile_definitions(xrEngine PRIVATE ENGINE_BUILD)
