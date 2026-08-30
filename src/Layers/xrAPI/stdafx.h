// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently

#pragma once

// This module only defines a handful of extern globals + trivial no-op
// stub functions (see xrAPI.cpp / render_stub.cpp) - it needs none of
// windows.h's real API surface, just the portable BOOL/HANDLE/DWORD/LPVOID
// stand-ins xrCore.h already provides for every other module on this port.
#include "../../xrCore/xrCore.h"
