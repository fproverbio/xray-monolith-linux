#pragma once

enum Estate
{
	clbNone =0,
	clbNearUp,
	clbNearDown,
	clbClimbingUp,
	clbClimbingDown,
	clbDepart,
	clbNoLadder,
	clbNoState
};

class IPhysicsShellHolder;

class IElevatorState
{
public:
	virtual Estate State() = 0;
	virtual void NetRelcase(IPhysicsShellHolder* O) = 0;
protected:
	virtual ~IElevatorState() = 0;
};

// GCC rejects a pure-specifier combined with a function-body in one
// member-declarator (MSVC accepts it as an extension) - split into a bare
// declaration above plus this out-of-class inline definition.
inline IElevatorState::~IElevatorState() {}
