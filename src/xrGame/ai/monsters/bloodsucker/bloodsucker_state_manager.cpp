#include "../../../StdAfx.h"
#include "bloodsucker_state_manager.h"
#include "bloodsucker.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../states/monster_state_rest.h"
#include "../states/monster_state_attack.h"
#include "../states/monster_state_panic.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_hear_int_sound.h"
#include "../states/monster_state_hear_danger_sound.h"
#include "../states/monster_state_hitted.h"

#include "bloodsucker_vampire.h"
#include "bloodsucker_predator.h"
#include "bloodsucker_state_capture_jump.h"
#include "bloodsucker_attack_state.h"


CStateManagerBloodsucker::CStateManagerBloodsucker(CAI_Bloodsucker* monster) : inherited(monster)
{
	this->add_state(eStateRest, xr_new<CStateMonsterRest<CAI_Bloodsucker>>(monster));
	this->add_state(eStatePanic, xr_new<CStateMonsterPanic<CAI_Bloodsucker>>(monster));

	this->add_state(eStateAttack, xr_new<CStateMonsterAttack<CAI_Bloodsucker>>(monster));
	//add_state(eStateAttack,				xr_new<CBloodsuckerStateAttack<CAI_Bloodsucker> >			(monster));

	this->add_state(eStateEat, xr_new<CStateMonsterEat<CAI_Bloodsucker>>(monster));
	this->add_state(eStateHearInterestingSound, xr_new<CStateMonsterHearInterestingSound<CAI_Bloodsucker>>(monster));
	this->add_state(eStateHearDangerousSound, xr_new<CStateMonsterHearDangerousSound<CAI_Bloodsucker>>(monster));
	this->add_state(eStateHitted, xr_new<CStateMonsterHitted<CAI_Bloodsucker>>(monster));
	this->add_state(eStateVampire_Execute, xr_new<CStateBloodsuckerVampireExecute<CAI_Bloodsucker>>(monster));
}

void CStateManagerBloodsucker::drag_object()
{
	CEntityAlive* const ph_obj = this->object->m_cob;
	if (!ph_obj)
	{
		return;
	}

	IKinematics* const kinematics = smart_cast<IKinematics*>(ph_obj->Visual());
	if (!kinematics)
	{
		return;
	}

	CMonsterSquad* const squad = monster_squad().get_squad(this->object);
	if (squad)
	{
		squad->lock_corpse(ph_obj);
	}

	{
		const u16 drag_bone = kinematics->LL_BoneID(this->object->m_str_cel);
		this->object->character_physics_support()->movement()->PHCaptureObject(ph_obj, drag_bone);
	}

	IPHCapture* const capture = this->object->character_physics_support()->movement()->PHCapture();

	if (capture && !capture->Failed() && this->object->is_animated())
	{
		this->object->start_drag();
	}
}

void CStateManagerBloodsucker::update()
{
	inherited::update();
}

bool CStateManagerBloodsucker::check_vampire()
{
	if (this->prev_substate != eStateVampire_Execute)
	{
		if (this->get_state(eStateVampire_Execute)->check_start_conditions()) return true;
	}
	else
	{
		if (!this->get_state(eStateVampire_Execute)->check_completion()) return true;
	}
	return false;
}

void CStateManagerBloodsucker::execute()
{
	u32 state_id = u32(-1);

	const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();

	if (enemy)
	{
		if (check_vampire())
		{
			state_id = eStateVampire_Execute;
		}
		else
		{
			switch (this->object->EnemyMan.get_danger_type())
			{
			case eStrong: state_id = eStatePanic;
				break;
			case eWeak: state_id = eStateAttack;
				break;
			}
		}
	}
	else if (this->object->HitMemory.is_hit())
	{
		state_id = eStateHitted;
	}
	else if (this->object->hear_interesting_sound)
	{
		state_id = eStateHearInterestingSound;
	}
	else if (this->object->hear_dangerous_sound)
	{
		state_id = eStateHearDangerousSound;
	}
	else
	{
		if (can_eat()) state_id = eStateEat;
		else state_id = eStateRest;
	}

	// check if start interesting sound state
	// 	if ( (prev_substate != eStateHearInterestingSound) && (state_id == eStateHearInterestingSound) )
	// 	{
	// 		object->start_invisible_predator();
	// 	} 
	// 	else
	// 	// check if stop interesting sound state
	// 	if ( (prev_substate == eStateHearInterestingSound) && (state_id != eStateHearInterestingSound) ) 
	// 	{
	// 		object->stop_invisible_predator();
	// 	}

	this->select_state(state_id);

	// выполнить текущее состояние
	this->get_state_current()->execute();

	this->prev_substate = this->current_substate;
}
