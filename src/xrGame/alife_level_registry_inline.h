////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_level_registry_inline.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife level registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

// psAI_Flags/aiALife - real global debug-flags declared in ai_debug.h;
// this file uses them without including it (arrived transitively through
// PCH context on Windows, same class of gap as notes section 27a's
// DPNSEND_GUARANTEED - real definition lives in CustomMonster.cpp, not yet
// ported, but the extern declaration is all a compile needs here).
// ai_debug.h's own `extern Flags32 psAI_Flags;` is inside `#ifndef
// MASTER_GOLD` - genuinely dead in every translation unit of this build:
// xrCore.h's own MASTER_GOLD `#ifndef DEBUG` check runs before that same
// file's later `#ifdef _DEBUG #define DEBUG` a few lines down, so
// MASTER_GOLD always ends up defined regardless of the real _DEBUG compile
// flag - a genuine pre-existing ordering quirk in xrCore.h, out of scope
// to touch here (core, already-proven-compiling file, wide blast radius).
// Declared directly instead, matching the same local `extern Flags32
// psAI_Flags;` pattern already used successfully elsewhere in this
// codebase (e.g. Level.cpp/script_engine.cpp/alife_trader_abstract.cpp).
#include "ai_debug.h"
#include "ai_space.h"

extern Flags32 psAI_Flags;

IC CALifeLevelRegistry::CALifeLevelRegistry(const GameGraph::_LEVEL_ID& level_id)
{
	m_level_id = level_id;
}

IC GameGraph::_LEVEL_ID CALifeLevelRegistry::level_id() const
{
	return (m_level_id);
}

IC void CALifeLevelRegistry::add(CSE_ALifeDynamicObject* object)
{
	if (ai().game_graph().vertex(object->m_tGraphID)->level_id() != level_id())
		return;

#ifdef DEBUG
	if (psAI_Flags.test(aiALife)) {
		Msg				("[LSS] adding object [%s][%d] to current level",object->name_replace(),object->ID);
	}
#endif
	inherited::add(object->ID, object);
}

IC void CALifeLevelRegistry::remove(CSE_ALifeDynamicObject* object, bool no_assert)
{
#ifdef DEBUG
	if (psAI_Flags.test(aiALife)) {
		Msg				("[LSS] removing object [%s][%d] from current level",object->name_replace(),object->ID);
	}
#endif
	inherited::remove(object->ID, no_assert);
}

template <typename _update_predicate>
IC void CALifeLevelRegistry::update(const _update_predicate& predicate, bool const iterate_as_first_time_next_time)
{
	//	u32					object_count = 
	inherited::update(predicate, iterate_as_first_time_next_time);
#ifdef FULL_LEVEL_UPDATE
	m_first_update		= true;
#endif
#ifdef DEBUG
	if (psAI_Flags.test(aiALife)) {
//		Msg				("[LSS][OOS][%d : %d]",object_count, objects().size());
	}
#endif
}

IC CSE_ALifeDynamicObject* CALifeLevelRegistry::object(const ALife::_OBJECT_ID& id, bool no_assert) const
{
	_REGISTRY::const_iterator I = objects().find(id);
	if (I == objects().end())
	{
		THROW2(no_assert, "The spesified object hasn't been found in the current level!");
		return (0);
	}
	return ((*I).second);
}
