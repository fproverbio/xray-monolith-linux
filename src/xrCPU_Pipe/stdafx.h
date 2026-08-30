// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently

#pragma once

// Real MSVC precompiled header for this module is StdAfx.h (capital S) -
// every real .cpp file here already #includes "stdafx.h" in lowercase
// (matching the general project-wide case-sensitive #include bug class),
// which never matched StdAfx.h on this case-sensitive filesystem at all.
// This file - not a fix to those #include directives - is the portable
// replacement content, following the same "xrCore.h instead of
// <windows.h>" pattern every other small module on this port uses (e.g.
// xrNetServer's stdafx.h). StdAfx.h/.cpp (capital S) are now dead/unused.
#define MTL_EXPORT_API
#define ENGINE_API
#define DLL_API
#define ECORE_API

#include "../xrCore/xrCore.h"
