////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_type.h
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script sound type
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"
#include "ai_sounds.h" // ESoundTypes is only forward-declared without this; real declaration lives here

typedef enum_exporter<ESoundTypes> CScriptSoundType;
