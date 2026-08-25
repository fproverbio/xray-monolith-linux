#pragma once

class ICollisionDamageReceiver
{
public:

	virtual void CollisionHit(u16 source_id, u16 bone_id, float power, const Fvector& dir, Fvector& pos) =0;
protected:
	// MSVC accepts a pure-specifier combined with an inline function-body
	// on the same declaration (`=0 { }`); standard C++ doesn't ("pure-
	// specifier on function-definition" under GCC/Clang) - split into a
	// pure declaration plus a separate out-of-line empty definition,
	// the portable way to give a pure virtual destructor a (required,
	// since derived dtors call it) body.
	virtual ~ICollisionDamageReceiver() =0;
};

inline ICollisionDamageReceiver::~ICollisionDamageReceiver()
{
}

struct dContact;
struct SGameMtl;
XRPHYSICS_API void DamageReceiverCollisionCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1,
                                                   SGameMtl* material_2);
XRPHYSICS_API void BreakableObjectCollisionCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1,
                                                    SGameMtl* material_2);
