#pragma once

#define	MTL_EXPORT_API
#define ENGINE_API
#define DLL_API		
#define ECORE_API
//#include "../xrEngine/stdafx.h"

#include "../xrCore/xrCore.h"

#include "../xrServerEntities/smart_cast.h"
//#include "../xrEngine/pure.h"
//#include "../xrEngine/engineapi.h"
//#include "../xrEngine/eventapi.h"


#include "../xrCDB/xrCDB.h"
#include "../xrSound/Sound.h"
//#include "../xrEngine/IGame_Level.h"

#pragma comment( lib, "xrCore.lib"	)

#include "xrPhysics.h"

#include "../Include/xrAPI/xrAPI.h"
// Guarded under _WIN32 too, matching every other real-but-Windows-only
// D3D include found so far in this port (xrEngine/stdafx.h's <d3d9.h>,
// fmesh.cpp's <d3dx9.h> - see notes section 22a/24c) - this build never
// actually defines plain DEBUG (only _DEBUG, see CMake/XRay.Compiler.cmake),
// so this was already dead code under this port's CMake config, but the
// _WIN32 guard makes that non-accidental rather than relying on a define
// that happens to not be set.
#if defined(_WIN32) && defined(DEBUG)
#include "d3d9types.h"
#endif
//IC IGame_Level &GLevel()
//{
//	VERIFY( g_pGameLevel );
//	return *g_pGameLevel;
//}
class CGameMtlLibrary;
IC CGameMtlLibrary& GMLibrary()
{
	VERIFY(PGMLib);
	return *PGMLib;
}
