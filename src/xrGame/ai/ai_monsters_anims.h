////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_monsters_anims.h
//	Created 	: 23.05.2003
//  Modified 	: 23.05.2003
//	Author		: Serge Zhem
//	Description : Animation templates for all of the monsters
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../Include/xrRender/KinematicsAnimated.h"
#include "../ai_debug.h"

#ifdef DEBUG
// psAI_Flags - see ai_monsters_misc.cpp's comment: MASTER_GOLD's
// #ifndef-DEBUG-before-DEBUG-is-#define'd ordering quirk in xrCore.h makes
// it always defined in this build, so ai_debug.h's own `extern Flags32
// psAI_Flags;` (guarded by #ifndef MASTER_GOLD) is dead here. Established
// fix: redeclare it locally wherever it's actually used.
extern Flags32 psAI_Flags;
#endif

DEFINE_VECTOR(MotionID, ANIM_VECTOR, ANIM_IT);

class CAniVector
{
public:
	ANIM_VECTOR A;

	void Load(IKinematicsAnimated* tpKinematics, LPCSTR caBaseName);
};

template <LPCSTR caBaseNames[]>
class CAniFVector
{
public:
	ANIM_VECTOR A;

	IC void Load(IKinematicsAnimated* tpKinematics, LPCSTR caBaseName)
	{
		A.clear();
		string256 S;
		int j = 0;
		for (; caBaseNames[j]; ++j);
		A.resize(j);
		for (int i = 0; i < j; ++i)
		{
			strconcat(sizeof(S), S, caBaseName, caBaseNames[i]);
			A[i] = tpKinematics->ID_Cycle_Safe(S);
#ifdef DEBUG
			if (A[i] && psAI_Flags.test(aiAnimation))
				Msg		("* Loaded animation %s",S);
#endif
		}
	}
};

template <class TYPE_NAME, LPCSTR caBaseNames[]>
class CAniCollection
{
public:
	xr_vector<TYPE_NAME> A;

	IC void Load(IKinematicsAnimated* tpKinematics, LPCSTR caBaseName)
	{
		A.clear();
		string256 S;
		int j = 0;
		for (; caBaseNames[j]; ++j);
		A.resize(j);
		for (int i = 0; i < j; ++i)
			A[i].Load(tpKinematics, strconcat(sizeof(S), S, caBaseName, caBaseNames[i]));
	}
};
