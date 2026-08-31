#pragma once

// Stand-in for mingw-w64's own <_mingw_unicode.h>. dxvk's public headers
// (src/3rd party/dxvk/include/native/directx/d3dcompiler.h) are written
// for a MinGW cross-compiler, where this header comes from the toolchain
// itself and provides the A/W (ANSI/wide) name-dispatch macros real
// Windows SDK headers use to pick FunctionA vs FunctionW based on the
// UNICODE define. Native Linux GCC has no such header. This reproduces
// the real macro semantics (not a _WIN32 fake-out) for the handful of
// names dxvk's headers actually reference; this build never defines
// UNICODE/_UNICODE, so the ANSI branch is always the one taken.
//
// Lives here rather than patching dxvk itself: it's a brand-new file
// with no upstream original to diverge from, and this directory is
// already ahead of dxvk's include/native/directx on the include path
// (see CMakeLists.txt), so the angle-bracket #include picks this up.

#if defined(UNICODE) || defined(_UNICODE)
#ifndef __MINGW_NAME_AW
#define __MINGW_NAME_AW(func) func##W
#endif
#ifndef __MINGW_NAME_AW_EXT
#define __MINGW_NAME_AW_EXT(func, ext) func##W##ext
#endif
#ifndef __MINGW_NAME_UAW
#define __MINGW_NAME_UAW(func) func##W
#endif
#else
#ifndef __MINGW_NAME_AW
#define __MINGW_NAME_AW(func) func##A
#endif
#ifndef __MINGW_NAME_AW_EXT
#define __MINGW_NAME_AW_EXT(func, ext) func##A##ext
#endif
#ifndef __MINGW_NAME_UAW
#define __MINGW_NAME_UAW(func) func##A
#endif
#endif
