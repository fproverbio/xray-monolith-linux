#pragma once
#include "PhysicsExternalCommon.h"

class IPHStaticGeomShell
{
protected:
	virtual ~IPHStaticGeomShell() = 0;

	//	virtual void						set_ObjectContactCallback	(ObjectContactCallbackFun* callback);
};

// GCC rejects a pure-specifier combined with a function-body in one
// member-declarator (MSVC accepts it as an extension) - split into a bare
// declaration above plus this out-of-class inline definition.
inline IPHStaticGeomShell::~IPHStaticGeomShell() {}

class IPhysicsShellHolder;
class IClimableObject;
XRPHYSICS_API IPHStaticGeomShell* P_BuildStaticGeomShell(IPhysicsShellHolder* obj,
                                                         ObjectContactCallbackFun* object_contact_callback);
XRPHYSICS_API IPHStaticGeomShell* P_BuildLeaderGeomShell(IClimableObject* obj, ObjectContactCallbackFun* callback,
                                                         const Fobb& b);
XRPHYSICS_API void DestroyStaticGeomShell(IPHStaticGeomShell* & p);

//CPHStaticGeomShell* P_BuildStaticGeomShell(CGameObject* obj,ObjectContactCallbackFun* object_contact_callback,Fobb &b);
//void				P_BuildStaticGeomShell(CPHStaticGeomShell* shell,CGameObject* obj,ObjectContactCallbackFun* object_contact_callback,Fobb &b);
