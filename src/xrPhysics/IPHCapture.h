#pragma once
class IPhysicsShellHolder;

class IPHCapture
{
public:
	virtual bool Failed() =0;
	virtual void RemoveConnection(IPhysicsShellHolder* O) = 0;
	virtual void Release() =0;
protected:
	virtual ~IPHCapture() = 0;
};

// GCC rejects a pure-specifier combined with a function-body in one
// member-declarator (MSVC accepts it as an extension) - split into a bare
// declaration above plus this out-of-class inline definition.
inline IPHCapture::~IPHCapture() {}

class CPHCharacter;
struct NearestToPointCallback;
XRPHYSICS_API IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object,
                                           NearestToPointCallback* cb /*=0*/);
XRPHYSICS_API IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object, u16 element);
XRPHYSICS_API void phcapture_destroy(IPHCapture* & c);
