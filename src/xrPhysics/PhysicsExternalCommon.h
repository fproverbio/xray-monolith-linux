#ifndef	_PHYSICS_EXTERNAL_COMMON_
#define _PHYSICS_EXTERNAL_COMMON_
#pragma once
// t_get_box() below calls cast_fv() on a plain `const float*` - not a
// dependent name (no template parameter involved in that particular
// call), so it must be declared before this template is even parsed,
// not just before it's instantiated. This header used to rely on
// whichever consumer happened to #include "MathUtils.h" first - MSVC's
// permissive two-phase-lookup deferred the check far enough that it
// never mattered there; GCC correctly rejects it. Same "first real
// instantiation reveals a latent MSVC-permissive header-ordering bug"
// shape already catalogued elsewhere in this port (Feel_Vision.cpp,
// notes section 24c) - include the real dependency directly instead.
#include "MathUtils.h"
struct dContactGeom;
struct dContact;
struct SGameMtl;

namespace CDB
{
	class TRI;
}

class CBoneInstance;

typedef void ContactCallbackFun(CDB::TRI* T, dContactGeom* c);
typedef void ObjectContactCallbackFun(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1,
                                      SGameMtl* material_2);


typedef void _BCL BoneCallbackFun(CBoneInstance* B);


typedef void PhysicsStepTimeCallback(u32 step_start, u32 step_end);
//extern			PhysicsStepTimeCallback		*physics_step_time_callback;
struct dxGeomUserData;
struct dContactGeom;
XRPHYSICS_API bool ContactShotMarkGetEffectPars(dContactGeom* c, dxGeomUserData* & data, float& vel_cret,
                                                bool& b_invert_normal);

template <typename geom_type>
void t_get_box(const geom_type* shell, const Fmatrix& form, Fvector& sz, Fvector& c)
{
	c.set(0, 0, 0);
	VERIFY(sizeof( form.i ) + sizeof( form._14_ ) == 4* sizeof( float ));
	for (int i = 0; 3 > i; ++i)
	{
		float lo, hi;
		const Fvector& ax = cast_fv(((const float*)&form + i * 4));
		shell->get_Extensions(ax, 0, lo, hi);
		sz[i] = hi - lo;
		c.add(Fvector().mul(ax, (lo + hi) / 2));
	}
}

enum ERestrictionType
{
	rtStalker =0,
	rtStalkerSmall,
	rtMonsterMedium,
	rtNone,
	rtActor
};
#endif
