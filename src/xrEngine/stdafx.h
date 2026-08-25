#ifndef STDAFX_3DA
#define STDAFX_3DA
#pragma once

#ifdef _EDITOR
#include "..\editors\ECore\stdafx.h"
#else

// INGAME_EDITOR was self-defined here for every non-NDEBUG (i.e. Debug)
// build, independent of the .vcxproj's own <PreprocessorDefinitions> -
// none of the 13 real Monolith configs define it there (see
// playground/xray-monolith-vulkan-port-notes.md section 21a point 5), but
// this local logic would otherwise still flip it on for a CMake Debug
// build (which doesn't define NDEBUG either). Its #ifdef targets
// (../include/editor/ide.hpp, src/editors/) don't exist anywhere in this
// checkout, so leave it permanently off rather than let a Debug build
// break on a dead editor-SDK dependency.
// #ifndef NDEBUG
// # ifndef INGAME_EDITOR
// # define INGAME_EDITOR
// # endif // #ifndef INGAME_EDITOR
// #endif // #ifndef NDEBUG

#ifdef INGAME_EDITOR
# define _WIN32_WINNT 0x0550
#endif // #ifdef INGAME_EDITOR

#include "../xrCore/xrCore.h"
#include "../Include/xrAPI/xrAPI.h"

#ifdef _DEBUG
# define D3D_DEBUG_INFO
#endif

// Real D3D9/DirectPlay8 headers, Windows-only - the renderer is being
// replaced with Vulkan (see notes section 20+) and DirectPlay8 was
// already dropped in xrNetServer (section 19). Neither exists on Linux;
// guarded rather than deleted, matching every other unconditional
// Windows-only #include found so far in this port.
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4995)
#include <d3d9.h>
//#include <dplay8.h>
#pragma warning(pop)
#endif

// you must define ENGINE_BUILD then building the engine itself
// and not define it if you are about to build DLL
#ifndef NO_ENGINE_API
#ifdef ENGINE_BUILD
#define DLL_API
//__declspec(dllimport)
#define ENGINE_API
//__declspec(dllexport)
#else
#undef DLL_API
#define DLL_API
//__declspec(dllexport)
#define ENGINE_API
//__declspec(dllimport)
#endif
#else
#define ENGINE_API
#define DLL_API
#endif // NO_ENGINE_API

#define ECORE_API

// Our headers
#include "Engine.h"
#include "defines.h"
#ifndef NO_XRLOG
#include "../xrCore/log.h"
#endif
#include "device.h"
#include "../xrCore/FS.h"

#include "../xrCDB/xrXRC.h"

#include "../xrSound/Sound.h"

extern ENGINE_API CInifile* pGameIni;

#pragma comment( lib, "xrCore.lib" )
#pragma comment( lib, "xrCDB.lib" )
#pragma comment( lib, "xrSound.lib" )

//AVO: lua re-org
#ifdef USE_LUAJIT_ONE //defined in project props
#pragma comment(lib, "LuaJIT-1.1.8.lib")
#else
#pragma comment(lib, "lua51.lib" )
#endif
//#include "lua/library_linkage.h"
//-AVO

#pragma comment( lib, "xrAPI.lib" )

#pragma comment( lib, "winmm.lib" )

#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "dinput8.lib" )
#pragma comment( lib, "dxguid.lib" )

#ifndef DEBUG
# define LUABIND_NO_ERROR_CHECKING
#endif

#if !defined(DEBUG) || defined(FORCE_NO_EXCEPTIONS)
# define LUABIND_NO_EXCEPTIONS
# define BOOST_NO_EXCEPTIONS
#endif

#define LUABIND_DONT_COPY_STRINGS

#define READ_IF_EXISTS(ltx,method,section,name,default_value)\
 (((ltx)->line_exist(section, name)) ? ((ltx)->method(section, name)) : (default_value))

#endif // !M_BORLAND
#endif // !defined STDAFX_3DA
