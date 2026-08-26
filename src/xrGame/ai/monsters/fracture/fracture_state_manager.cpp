#include "../../../StdAfx.h"
#include "fracture.h"
#include "fracture_state_manager.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../states/monster_state_rest.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_attack.h"
#include "../states/monster_state_panic.h"
#include "../states/monster_state_hear_danger_sound.h"
#include "../states/monster_state_hitted.h"

#include "../../../EntityCondition.h"

CStateManagerFracture::CStateManagerFracture(CFracture* obj) : inherited(obj)
{
	this->add_state(eStateRest, xr_new<CStateMonsterRest<CFracture>>(obj));
	this->add_state(eStateAttack, xr_new<CStateMonsterAttack<CFracture>>(obj));
	this->add_state(eStateEat, xr_new<CStateMonsterEat<CFracture>>(obj));
	this->add_state(eStateHearDangerousSound, xr_new<CStateMonsterHearDangerousSound<CFracture>>(obj));
	this->add_state(eStatePanic, xr_new<CStateMonsterPanic<CFracture>>(obj));
	this->add_state(eStateHitted, xr_new<CStateMonsterHitted<CFracture>>(obj));
}

CStateManagerFracture::~CStateManagerFracture()
{
}

void CStateManagerFracture::execute()
{
	u32 state_id = u32(-1);
	const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();

	if (enemy)
	{
		switch (this->object->EnemyMan.get_danger_type())
		{
		case eStrong: state_id = eStatePanic;
			break;
		case eWeak: state_id = eStateAttack;
			break;
		}
	}
	else if (this->object->HitMemory.is_hit())
	{
		state_id = eStateHitted;
	}
	else if (this->object->hear_interesting_sound || this->object->hear_dangerous_sound)
	{
		state_id = eStateHearDangerousSound;
	}
	else
	{
		if (can_eat()) state_id = eStateEat;
		else
		{
			// Rest & Idle states here 
			state_id = eStateRest;
		}
	}

	// установить текущее состояние
	this->select_state(state_id);

	// выполнить текущее состояние
	this->get_state_current()->execute();

	this->prev_substate = this->current_substate;
}
