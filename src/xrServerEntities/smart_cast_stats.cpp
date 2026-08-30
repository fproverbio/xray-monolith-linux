////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast_stats.cpp
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast statistics
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

// The original CSmartCastStats machinery (tracking every smart_cast<> call's
// from/to type pair under a SMART_CAST_STATS build flag) has been entirely
// commented out in this file since the upstream revision this port is based
// on - SMART_CAST_STATS/SMART_CAST_STATS_ALL are never defined anywhere in
// this tree (confirmed via grep), so the real tracking code was already
// permanently dead upstream, not something this port disabled. This file
// itself was correspondingly never compiled (absent from xrGame.vcxproj -
// only smart_cast.h is <ClInclude>-listed), which is why console_commands.cpp's
// "show_smart_cast_stats"/"clear_smart_cast_stats" debug console commands
// (registered under #ifdef DEBUG) have been an unnoticed link-time landmine
// for any Debug config that ever actually links a real executable.
//
// This keeps the file's original disabled-feature behavior (the commented-out
// #else branches below) as real, callable code, rather than porting the
// dead stats-tracking feature itself.
void show_smart_cast_stats()
{
	Msg("! SMART_CAST_STATS macros is not defined, stats is disabled");
}

void clear_smart_cast_stats()
{
	Msg("! SMART_CAST_STATS macros is not defined, stats is disabled");
}

void release_smart_cast_stats()
{
}
