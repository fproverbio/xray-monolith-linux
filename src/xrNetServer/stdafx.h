// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Third generation by Oles.

#ifndef stdafxH
#define stdafxH

#pragma once

#include "../xrCore/xrCore.h"

// <DPlay/dplay8.h> and the _RELEASE/_SHOW_REF COM-idiom macros dropped -
// only ever used by the real DirectPlay8 client/server code in
// NET_Client.cpp/NET_Server.cpp, both now dependency-free stubs (see
// notes §18 - multiplayer dropped as a concept per §11/§12/§13).

#include "NET_Shared.h"

#endif //stdafxH
