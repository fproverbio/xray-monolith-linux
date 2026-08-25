#pragma once
#include "game_cl_base.h"
#include "script_export_space.h"

class game_cl_Single : public game_cl_GameState
{
	typedef game_cl_GameState inherited;
public :
	game_cl_Single();
	virtual CUIGameCustom* createGameUI();
	virtual char* getTeamSection(int Team);
	virtual bool IsServerControlHits() { return true; };

	virtual ALife::_TIME_ID GetStartGameTime();
	virtual ALife::_TIME_ID GetGameTime();
	virtual float GetGameTimeFactor();
	virtual void SetGameTimeFactor(const float fTimeFactor);

	virtual ALife::_TIME_ID GetEnvironmentGameTime();
	virtual float GetEnvironmentGameTimeFactor();
	virtual void SetEnvironmentGameTimeFactor(const float fTimeFactor);

	void OnDifficultyChanged();
};


// game difficulty
enum ESingleGameDifficulty
{
	egdNovice = 0,
	egdStalker = 1,
	egdVeteran = 2,
	egdMaster = 3,
	egdCount,
	egd_force_u32 = u32(-1)
};

extern ESingleGameDifficulty g_SingleGameDifficulty;
// Missing `extern` - an incomplete-bound array with no initializer is only
// legal as a declaration referring to a real definition elsewhere (here,
// game_cl_single.cpp's own `xr_token difficulty_type_token[] = {...}`),
// never as a standalone definition. MSVC tolerated the omission; GCC
// doesn't ("storage size ... isn't known").
extern xr_token difficulty_type_token [ ];

typedef enum_exporter<ESingleGameDifficulty> CScriptGameDifficulty;
