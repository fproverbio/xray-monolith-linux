#include "../../../StdAfx.h"
#include "snork.h"
#include "snork_state_manager.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../../../Level.h"
#include "../../../level_debug.h"
#include "../states/monster_state_rest.h"
#include "../states/monster_state_attack.h"
#include "../states/monster_state_panic.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_hear_int_sound.h"
#include "../states/monster_state_hear_danger_sound.h"
#include "../states/monster_state_hitted.h"
#include "../states/state_look_point.h"
#include "../states/state_test_look_actor.h"
#include "../states/state_test_state.h"
#include "../states/monster_state_help_sound.h"

#include "../../../EntityCondition.h"

CStateManagerSnork::CStateManagerSnork(CSnork* obj) : inherited(obj)
{
	this->add_state(eStateRest, xr_new<CStateMonsterRest<CSnork>>(obj));
	this->add_state(eStatePanic, xr_new<CStateMonsterPanic<CSnork>>(obj));
	this->add_state(eStateAttack, xr_new<CStateMonsterAttack<CSnork>>(obj));
	this->add_state(eStateEat, xr_new<CStateMonsterEat<CSnork>>(obj));
	this->add_state(eStateHearInterestingSound, xr_new<CStateMonsterHearInterestingSound<CSnork>>(obj));
	this->add_state(eStateHearDangerousSound, xr_new<CStateMonsterHearDangerousSound<CSnork>>(obj));
	this->add_state(eStateHitted, xr_new<CStateMonsterHitted<CSnork>>(obj));

	this->add_state(eStateFindEnemy, xr_new<CStateMonsterTestCover<CSnork>>(obj));
	this->add_state(eStateHearHelpSound, xr_new<CStateMonsterHearHelpSound<CSnork>>(obj));
}

CStateManagerSnork::~CStateManagerSnork()
{
}

void CStateManagerSnork::execute()
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
	else if (check_state(eStateHearHelpSound))
	{
		state_id = eStateHearHelpSound;
	}
	else if (this->object->hear_dangerous_sound)
	{
		state_id = eStateHearDangerousSound;
	}
	else if (this->object->hear_interesting_sound)
	{
		state_id = eStateHearInterestingSound;
	}
	else
	{
		if (can_eat()) state_id = eStateEat;
		else state_id = eStateRest;
	}


	//state_id = eStateFindEnemy;

	this->select_state(state_id);

	if ((this->current_substate == eStateAttack) && (this->current_substate != this->prev_substate))
	{
		this->object->start_threaten = true;
	}

	// выполнить текущее состояние
	this->get_state_current()->execute();

	this->prev_substate = this->current_substate;
}
