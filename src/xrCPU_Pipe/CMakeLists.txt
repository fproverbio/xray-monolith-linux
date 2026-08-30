# xrCPU_Pipe - a real, never-built-at-all module discovered while
# investigating the exploratory executable's residual undefined symbols
# (xrBind_PSGP/ttapi_Done, called directly from xrEngine/Engine.cpp).
# Same "only ever reached via headers, .cpp never added to any
# CMakeLists" shape as xrServerEntities/xrAPI before this port wired them
# in. See playground/xray-monolith-vulkan-port-notes.md for the full
# investigation.
#
# Real, portable code wired in: xrSkin2W.cpp (4 real x86 scalar CPU
# vertex-skinning fallbacks - vertRender/vertBoned1W..4W/CBoneInstance
# are all already-available types, no renderer needed to compile them),
# xrSkin2W_thread.cpp (splits xrSkin4W across ttapi's worker pool),
# ttapi.cpp (the worker-thread pool itself - ported from CreateThread/
# HANDLE/WaitForMultipleObjects to std::thread; this fork's own devs had
# already modernized its core-count detection to
# std::thread::hardware_concurrency(), only the actual OS thread
# creation/join/naming needed porting), xrCPU_Pipe.cpp (the xrBind_PSGP
# binder itself, unmodified besides adding real includes and dropping an
# unused DllMain stub).
#
# xrSkin2W_SSE.cpp is NOT built: pure 32-bit-only inline MASM __asm
# (registers named eax/esi/edx), doubly moot since xrCPU_Pipe.cpp's own
# SSE dispatch branch is already `#if 0`'d out upstream - only the x86
# scalar path is ever actually selected.
#
# PLC.cpp (the real PLC_calc3_x86 lighting-contribution calculation) is
# NOT built either - its signature takes a real `light&`/`CRenderDevice&`
# from the still out-of-scope renderer (Layers/xrRender/light.h). Its
# only real call site is Layers/xrRenderPC_R1/LightShadows.cpp, itself
# part of the unbuilt renderer stack, so it's unreachable in this build
# regardless. PLC_stub.cpp supplies a genuine no-op PLC_calc3_x86 instead
# (forward-declaring CRenderDevice/light is enough - a function that
# never dereferences a reference/pointer parameter doesn't need the
# complete type), matching the exact same "stub what's still coupled to
# the out-of-scope renderer" treatment as Layers/xrAPI/render_stub.cpp.
add_library(xrCPU_Pipe STATIC)

target_include_directories(xrCPU_Pipe PUBLIC .)

target_sources_grouped(
  TARGET xrCPU_Pipe
  NAME "CPUPipe"
  FILES
  PLC_stub.cpp
  ttapi.cpp
  xrCPU_Pipe.cpp
  xrSkin2W.cpp
  xrSkin2W_thread.cpp
)

target_link_libraries(xrCPU_Pipe PUBLIC xrCore)
