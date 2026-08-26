#pragma once
#include "state_manager.h"
#include "state.h"

// Lain: added
#ifdef DEBUG
#include "debug_text_tree.h"
#endif

template <typename _Object>
class CMonsterStateManager : public IStateManagerBase, public CState<_Object>
{
	typedef CState<_Object> inherited;

public:
	CMonsterStateManager(_Object* obj) : inherited(obj)
	{
	}

	virtual void reinit();
	virtual void update();
	virtual void force_script_state(EMonsterState state);
	virtual void execute_script_state();
	virtual void critical_finalize();
	// Was `virtual void remove_links(CObject* object) = 0 { ... }` - illegal
	// combined pure-specifier+body (bug pattern #7 in the port catalog,
	// previously only seen on destructors). MSVC accepts it as a nonstandard
	// extension; GCC rejects it outright, and since this is a template the
	// error poisons every instantiation ("instantiating erroneous template"
	// at every derived monster's CMonsterStateManager<T> use). Split into a
	// pure declaration here + an out-of-class definition below, so derived
	// classes must still override it but can still call the default body via
	// `CMonsterStateManager::remove_links(...)`.
	virtual void remove_links(CObject* object) = 0;

	virtual EMonsterState get_state_type();

	virtual bool check_control_start_conditions(ControlCom::EControlType type)
	{
		return inherited::check_control_start_conditions(type);
	}

	// Lain: added
#ifdef DEBUG
	virtual void    add_debug_info          (debug::text_tree& root_s);
#endif

protected:
	bool can_eat();
	bool check_state(u32 state_id);
};

#include "monster_state_manager_inline.h"
