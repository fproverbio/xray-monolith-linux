#ifndef XRCORE_PLATFORM_H
#define XRCORE_PLATFORM_H
#pragma once

// _WIN32 is the compiler-native "actually targeting Windows" signal (set
// by MSVC/MinGW/clang-cl), distinct from the project's own WIN32 macro
// which several files still check as a general "this is the Windows-era
// codebase" compatibility flag during the Linux port (see
// playground/xray-monolith-vulkan-port-notes.md). Only _WIN32 gates the
// actual windows.h pull below - on native GCC/Clang Linux builds this
// whole block is skipped.
#ifdef _WIN32

#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#ifndef STRICT
# define STRICT // Enable strict syntax
#endif // STRICT
#define IDIRECTPLAY2_OR_GREATER // ?
#define DIRECTINPUT_VERSION 0x0800 //
#define _CRT_SECURE_NO_DEPRECATE // vc8.0 stuff, don't deprecate several ANSI functions

// windows.h
#undef _WIN32_WINNT

#ifndef _WIN32_WINNT
#ifdef _MSC_VER
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#else // ifdef _MSC_VER
#define _WIN32_WINNT 0x0501
#endif // ifdef _MSC_VER
#endif // ifndef _WIN32_WINNT

#ifdef __BORLANDC__
#include <vcl.h>
#include <mmsystem.h>
#include <stdint.h>
#endif

#define NOGDICAPMASKS
//#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NODRAWTEXT
#define NOMEMMGR
#define NOMETAFILE
#define NOSERVICE
#define NOCOMM
#define NOHELP
#define NOPROFILER
#define NOMCX
#define NOMINMAX
#define DOSWIN32
#define _WIN32_DCOM

#pragma warning(push)
#pragma warning(disable:4005)
#include <windows.h>
#ifndef __BORLANDC__
#include <windowsx.h>
#endif
#pragma warning(pop)

#else // !_WIN32

// windows.h transitively provided a huge surface (basic types, file I/O,
// synchronization primitives, ...) that plenty of files in this codebase
// rely on without their own explicit include - these three headers are
// the portable replacement for that surface, real implementations not
// stubs (see each file's own header comment, and
// playground/xray-monolith-vulkan-port-notes.md section 14).
#include "win32_compat.h"
#include "posix_filemap.h"
#include "posix_sync.h"
#include "posix_lowio.h"

#endif // _WIN32

#endif
