cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(XRay.Configurations)
include(utils)

project(
  Anomaly
  DESCRIPTION "X-Ray Monolith Edition for S.T.A.L.K.E.R. Anomaly (Linux/Vulkan port)"
  LANGUAGES CXX C
)

message(STATUS "X-Ray Monolith - Linux/Vulkan port")
message(STATUS "  CMake:    ${CMAKE_VERSION}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build:    ${CMAKE_BUILD_TYPE}")

include(XRay.Compiler)

# Matches src/Common.props' solution-wide <IncludePath> (applied to every
# .vcxproj via MSBuild props inheritance) - src/3rd party and sdk/include
# hold vendored third-party headers referenced by bare `<lib/header.h>`
# includes throughout the tree, not just from their own module.
include_directories(
  "${CMAKE_SOURCE_DIR}/src/3rd party"
  "${CMAKE_SOURCE_DIR}/sdk/include"
)

# Vendored 3rd-party libs that need their own build (not header-only,
# not folded into another module's sources like LZO is inside xrCore).
add_subdirectory(sdk/include/ogg/src)
add_subdirectory(sdk/include/vorbis/lib)
add_subdirectory(sdk/include/theora/lib)
add_subdirectory(sdk/include/giflib)
add_subdirectory(sdk/include/imgui)
add_subdirectory("src/3rd party/OpenAL-new")
add_subdirectory("src/3rd party/luajit-2")
add_subdirectory("src/3rd party/Vulkan")
add_subdirectory("src/3rd party/ode")

# glslang (Khronos reference GLSL/HLSL -> SPIR-V compiler), pinned to the
# same vulkan-sdk-1.4.321.0 tag as src/3rd party/Vulkan above to keep the
# whole Vulkan toolchain internally consistent. Not consumed by anything
# yet (no renderer backend exists to compile shaders for), vendored ahead
# of need since it's fully independent, low-risk work - see
# src/3rd party/glslang/NOTES.txt for the full rationale. ENABLE_HLSL is
# on since Monolith's existing shaders are HLSL; ENABLE_OPT is off since
# SPIRV-Tools isn't vendored (upstream itself just warns that HLSL-
# generated SPIR-V may be unvalidated without it - fine for now, revisit
# if that ever matters).
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL ON CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory("src/3rd party/glslang")

# SDL2's own real upstream CMakeLists.txt (src/3rd party/SDL2/) is used
# verbatim via add_subdirectory - configured entirely through cache
# variables rather than a hand-written wrapper. Audio is off (OpenAL-Soft
# already owns audio in this port). X11/OpenGL are left requested ON so
# they activate for free once real dev headers exist on the build machine
# (none are present right now - verified empirically, /usr/include/X11 is
# an empty stub directory, no Xlib.h/GL/gl.h anywhere - SDL2's own CMake
# gracefully resolves both to OFF rather than erroring, falling back to
# the offscreen/dummy video drivers). Vulkan/OpenGLES support both build
# clean regardless since SDL2 bundles its own Khronos headers for those
# and loads the actual libraries via dlopen at runtime, same approach as
# volk (src/3rd party/Vulkan) above.
#
# SDL_SHARED is ON (not the usual static-everything convention for this
# project - see BUILD_SHARED_LIBS below) because dxvk-native's own build
# links SDL2 as well, for window/surface interop (SDL_Vulkan_CreateSurface
# etc.). If the engine linked a static SDL2 and dxvk-native linked a
# separately-built one, the process would end up with two independent SDL2
# instances, each with its own internal window/video-device globals - a
# window created through one instance would not be valid to the other.
# Building our vendored SDL2 as a shared library and pointing dxvk-native's
# build at that exact .so (rather than any system SDL2) makes both link
# and load the same single instance at runtime. SDL2::SDL2 (used by
# xrEngine, see src/xrEngine/CMakeLists.txt) automatically aliases this
# shared target once SDL_SHARED is ON. SDL_STATIC is left ON too since
# nothing else in the tree depends on it being off.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_X11 ON CACHE BOOL "" FORCE)
set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN ON CACHE BOOL "" FORCE)
set(SDL_OPENGL ON CACHE BOOL "" FORCE)
set(SDL_OPENGLES ON CACHE BOOL "" FORCE)
add_subdirectory("src/3rd party/SDL2")

# luabind's own CMakeLists.txt (src/3rd party/luabind/) expects this to
# select the LuaJIT link, and defaults BUILD_SHARED_LIBS to ON itself -
# force it to match every other target in this tree (static).
set(XRAY_USE_LUAJIT ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("src/3rd party/luabind")

# lua-extensions - luaopen_lua_extensions() (script_storage.cpp) is called
# unconditionally on every Lua VM init, a real, mandatory startup
# dependency, not optional - see src/3rd party/lua-extensions/CMakeLists.txt
# for the full investigation (why it was never wired in, and why its two
# debug-only functions are stubbed rather than the whole module ported).
add_subdirectory("src/3rd party/lua-extensions")

# DirectXTex - texture loading/DDS/format-conversion library used by the DX11
# (xrRenderPC_R4) renderer tier in place of the D3DX9/D3DX11 texture
# utilities the original engine relied on (removed from the DirectX SDK long
# ago). BUILD_TOOLS/BUILD_SAMPLE/BUILD_DX12 are all Windows-toolchain-only
# (texassemble/texconv/texdiag, the DDSView sample, and the D3D12 interop
# path - nothing here uses D3D12) so they're forced off. libjpeg/libpng
# support is forced on: DirectXTexWIC.cpp (Windows Imaging Component) is
# WIN32-only (see src/3rd party/DirectXTex/CMakeLists.txt line ~124), so
# JPEG/PNG loading on Linux goes through DirectXTexJPEG.cpp/DirectXTexPNG.cpp
# instead, which need libjpeg-dev/libpng-dev installed (apt install
# libjpeg-dev libpng-dev). XR_DIRECTXTEX_DXVK_NATIVE is this project's own,
# honest macro (not a faked _WIN32 - see CMake/XRay.Compiler.cmake for why
# that's specifically avoided) that steers DirectXTexP.h/DirectXTex.h to
# reuse dxvk-native's own D3D11/COM headers, matching what xrRenderPC_R4
# compiles against, instead of DirectX-Headers' independent (and
# incompatible) wsl/wrladapter.h COM reimplementation.
set(XR_DIRECTXTEX_DXVK_NATIVE ON CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
set(BUILD_DX12 OFF CACHE BOOL "" FORCE)
set(ENABLE_LIBJPEG_SUPPORT ON CACHE BOOL "" FORCE)
set(ENABLE_LIBPNG_SUPPORT ON CACHE BOOL "" FORCE)
add_subdirectory("src/3rd party/DirectXTex")

add_subdirectory(src)
