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
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
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

add_subdirectory(src)
