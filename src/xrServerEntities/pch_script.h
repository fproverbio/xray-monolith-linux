////////////////////////////////////////////////////////////////////////////
//	Module 		: pch_script.h
//	Created 	: 23.05.2007
//  Modified 	: 10.08.2007
//	Author		: Dmitriy Iassenev
//	Description : precompiled header for lua and luabind users
////////////////////////////////////////////////////////////////////////////

#ifndef PCH_SCRIPT_H
#define PCH_SCRIPT_H

// Case-sensitivity fix (see notes file, xrGame section): on Windows'
// case-insensitive filesystem this resolved to whichever module's own
// stdafx-equivalent was reachable via that module's AdditionalIncludeDirectories
// (xrEngine's own file happens to be genuinely lowercase stdafx.h, so it was
// never exercised there); every real, on-disk consumer of pch_script.h
// (~280 files, all under src/xrGame) expects this to resolve to
// src/xrGame/StdAfx.h, so that's what's spelled out explicitly here.
#include "StdAfx.h"


//AVO: lua re-org
#include "lua.hpp"
/*extern "C" {
	#include <lua/lua.h>
	#include <lua/lualib.h>
	#include <lua/lauxlib.h>
}*/
//-AVO

#pragma warning(push)
#pragma warning(disable:4995)
#include <luabind/luabind.hpp>
#pragma warning(pop)

#include <luabind/object.hpp>
#include <luabind/functor.hpp>
#include <luabind/operator.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/iterator_policy.hpp>

#endif // PCH_SCRIPT_H
