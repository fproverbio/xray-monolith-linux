# First xrGame batch (see playground/xray-monolith-vulkan-port-notes.md,
# section 27) - xrGame is the last wholly-unstarted major module (~1239
# top-level .cpp files per the real xrGame.vcxproj <ClCompile> count). This
# is the very first batch, same "scope conservatively, verify real deps via
# #include grep, exclude what's genuinely blocked with a reasoning comment"
# discipline as every prior module's first batch (xrEngine's section 21/22).
#
# GameObject.h/GameObject.cpp are the base class for the whole gameplay
# object hierarchy and the recommended starting point (a prior structural
# survey - see section 26e). GameObject.h itself is genuinely tractable: its
# own 6 direct includes (xrServer_Space.h, alife_space.h, UsableScriptObject.h,
# script_binder.h, Hit.h, game_object_space.h) are all small, self-contained
# leaf headers (re-verified this pass via a real recursive #include closure
# script, not just trusted from the prior survey) - only 10 files total,
# 4 of them in xrServerEntities. It compiles clean below, as a byproduct of
# every .cpp in this batch #include-ing it.
#
# GameObject.cpp itself is a different story and is EXCLUDED from this batch,
# with real evidence, not a guess: a recursive #include closure over its own
# 30-ish direct includes (script_game_object.h, ai_space.h, CustomMonster.h,
# game_sv_single.h, level_graph.h, game_cl_mp.h, HangingLamp.h, physicobject.h,
# etc.) reaches 264 files, 208 of them inside xrGame - Actor.h, Car.h,
# CustomMonster.h, Entity.h, Inventory.h, Level.h among them - i.e. it
# directly reaches almost the same hub territory the survey explicitly
# warned off (script_game_object.h, 156 includers). Porting GameObject.cpp
# for real means porting a large fraction of xrGame's core gameplay-object
# graph at the same time - a real, large, future pass, not a first-batch
# mechanical fix. Deferred.
#
# Instead, this batch proves GameObject.h compiles for real by porting a
# handful of genuinely near-leaf direct includers of GameObject.h (found via
# `grep -rli` for GameObject.h across src/, then ranked by their own real
# recursive #include closure size using the same script) - the smallest,
# most self-contained ones, not chasing deep chains. See per-file comments
# below for what each one needed.
add_library(xrGame STATIC)

target_include_directories(xrGame PUBLIC . ../xrServerEntities)

target_compile_definitions(xrGame PRIVATE
  XRGAME_EXPORTS
  dSINGLE
)

# StdAfx.h (this module's real precompiled-header source, matching every
# other module's stdafx.h/StdAfx.h) just forwards to xrEngine's own
# stdafx.h plus a couple of physics/profiler headers plus pch_script.h
# (lua/luabind) - already-ported territory, no changes needed beyond the
# per-file case-sensitivity fixes noted below. DPlay/dplay8.h - real
# DirectPlay8 header - is guarded out the same way xrNetServer's own
# stdafx.h already handles it (see notes section 12/13/18/19): edited in
# place here too, dropping the #include and the two now-orphaned
# GP_*_LEN defines' original DirectPlay8-profile purpose (they're just
# harmless numeric constants now, kept since other files may still
# reference them).
#
# detail_path_manager.cpp was in the original candidate list (a small
# 21-file closure via its unconditional includes) but got dropped after
# real testing: it has a `#ifdef DEBUG #include "custommonster.h" #endif`
# block, and DEBUG *is* live in this build (xrCore.h #defines DEBUG under
# _DEBUG, which this target's Debug config sets) - not a dead branch we
# can ignore. CustomMonster.h's own closure is 47 xrGame files on its own
# (ai/monsters territory), well past this batch's scope. Deferred to
# whichever future pass tackles CustomMonster.h for real.
target_sources_grouped(
  TARGET xrGame
  NAME "Game"
  FILES
  ai/monsters/energy_holder.cpp
  shootingObject_dump_impl.cpp
  moving_bones_snd_player.cpp
  aimers_base.cpp
  PHShellCreator.cpp
  danger_object_location.cpp
  doors_manager.cpp
  location_manager.cpp
  IKFoot.cpp
  animation_script_callback.cpp
  ai_obstacle.cpp
  obstacles_query.cpp
)

# Second batch (see playground/xray-monolith-vulkan-port-notes.md, section
# 28) - candidates found via a real recursive #include-closure script (see
# scratchpad, not committed - same discipline as batch 1/section 27),
# ranked by how many files in each candidate's own closure were still
# genuinely new/unproven vs. already reachable from batch 1's proven set.
# Unlike batch 1, none of these route through GameObject.h/.cpp at all -
# an independent near-leaf cluster (utility/debug/physics-scripted files),
# matching this pass's brief to also look beyond the GameObject hub.
#
# Major finding this batch, affecting far more than these files: ~686 of
# ~1239 top-level xrGame .cpp files have their own first #include line
# spelled lowercase #include "stdafx.h" instead of the real on-disk
# "StdAfx.h". On Windows this is a harmless case-insensitive match to this
# module's own real StdAfx.h; on Linux, quote-include search checks the
# includer's own directory case-*sensitively* first (fails), then falls
# through the full flat -I list in link order - and finds a *different*,
# genuinely-existing file, xrCore/stdafx.h (a real but much smaller header,
# xrCore's own PCH stub - it exists, so this never becomes a "file not
# found" error). The result: any of these files that don't happen to need
# anything from xrGame's real StdAfx.h chain (ENGINE_API, GameFont.h,
# xr_object.h, IGame_Level.h, xrPhysics.h, smart_cast.h, profiler.h,
# pch_script.h) will silently compile "successfully" against entirely the
# wrong precompiled-header content - not a compile error, just quietly
# wrong, and easy to mistake for "this file has no real dependencies" when
# ranking candidates. Confirmed via this batch's own first attempt: 12 of
# 17 zero-new-dependency-ranked candidates initially built "clean" this
# way; the other 5 (ParticlesPlayer.cpp, ai_sounds.cpp, danger_location.cpp,
# level_graph.cpp, plus several from the one-new-dependency tier) hard-
# errored with cascading "incomplete type"/"not declared" errors through
# xrEngine's own already-proven headers (PS_instance.h/ISheduled.h/
# IRenderable.h/Render.h/etc.) the moment their own content actually needed
# something from the real chain. Every file in this batch that had the
# lowercase spelling was fixed in place (one-line #include correction, not
# listed individually below - see git diff). Worth re-checking for every
# future batch pulled from the remaining ~1225: grep the first #include
# line of any new candidate .cpp before trusting a "0 new files" ranking.
#
# Second finding worth flagging for future batches: the real xrGame.vcxproj
# marks 3 top-level .cpp files <ExcludedFromBuild>true</ExcludedFromBuild>
# on Windows too - debug_make_final_test.cpp, static_cast_checked_test.cpp
# (both deliberate "won't compile" negative tests exercising a
# private-inheritance/friend "final class" trick, see their own comments -
# genuinely meant to fail, not a port bug), and DynamicHeightMap.cpp (not
# otherwise touched this batch). A candidate list built from the raw
# <ClCompile> tag list (as both batches' scripts did) won't see this flag -
# worth checking explicitly, not just closure-ranking, before trusting a
# candidate.
target_sources_grouped(
  TARGET xrGame
  NAME "Game2"
  FILES
  Actor_Sleep.cpp
  ai_debug_variables.cpp
  CameraEffector.cpp
  date_time.cpp
  game_sv_base_console_vars.cpp
  level_map_locations.cpp
  player_name_modifyer.cpp
  StdAfx.cpp
  ai_sounds.cpp
  poses_blending.cpp
  magic_box3.cpp
  danger_location.cpp
  level_graph.cpp
  ParticlesPlayer.cpp
  animation_movement_controller.cpp
  DamagableItem.cpp
  DelayedActionFuse.cpp
  game_sv_event_queue.cpp
  PhysicsGamePars.cpp
  random32.cpp
  secure_messaging.cpp
  stalker_animation_names.cpp
  animation_utils.cpp
  visual_memory_params.cpp
  fs_registrator_script.cpp
  script_render_device_script.cpp
  physics_element_scripted.cpp
  physics_shell_animated.cpp
  interactive_motion.cpp
)

# atlas_stalkercoppc_v1.c: EXCLUDED - its own single #include,
# atlas_stalkercoppc_v1.h, genuinely does not exist anywhere in this
# checkout (confirmed via `find` across the whole repo) despite being
# listed in xrGame.vcxproj's own <ClInclude> list - a real gap in this
# source tree, not a case-sensitivity issue. Reconstructing ~400 missing
# KEY_* enum constants from this file's own usage is possible but not a
# mechanical fix; deferred.
#
# script_engine_help.cpp: EXCLUDED - a lua-console debug-dump helper (lists
# every registered class/free-function) that uses several luabind
# reflection APIs genuinely absent from this vendored deboostified fork,
# confirmed by grep against the real headers, not assumption:
# luabind::detail::free_functions (namespace doesn't exist at all - the
# only hit anywhere under src/3rd party/luabind is a stale comment),
# luabind::internal_string, object::pushvalue()/object::set(),
# class_registry::iterate_classes() (real API is get_classes(), returning
# a map with different iteration semantics, not a drop-in rename like the
# get_globals -> globals case in notes section 25c/27d). A real rewrite
# against the actual vendored API, not a mechanical port; deferred.
#
# RegistryFuncs.cpp: EXCLUDED - a genuine Win32 Registry API wrapper
# (HKEY/RegOpenKeyEx/RegQueryValueEx/RegCloseKey, storing STALKER-Anomaly
# settings under HKEY_LOCAL_MACHINE) with no Linux equivalent - same
# "real platform gap, not a mechanical port" treatment as DirectPlay8/the
# Win32 GDI dedicated-server console (notes section 12/13/18/19/27).
# Deferred to whichever future pass redesigns config storage.
#
# debug_make_final_test.cpp / static_cast_checked_test.cpp: EXCLUDED -
# confirmed via xrGame.vcxproj itself (grep for ExcludedFromBuild, not
# assumed): both are deliberate negative compile-tests, excluded from the
# real Windows build too. debug_make_final_test.cpp's own comment says
# "the next 2 lines won't compile" - and they don't, by design (a
# private-inheritance "make this class un-derivable outside its friends"
# trick; GCC's "use of deleted function C::C()" on the deliberately-illegal
# derived class C is the test passing, not a port bug). Not added here;
# static_cast_checked.hpp itself (their shared header, not test-only - genuinely used by
# Actor_Movement.cpp/Weapon.cpp/Inventory.cpp/player_hud.cpp and others)
# still got real fixes below since future batches will need it working.

# xrCore/xrCDB/xrSound/xrEngine/xrPhysics: real header dependencies, verified
# via #include grep (not just carried over from the .vcxproj's
# <ProjectReference> list - see notes section 21a's established
# methodology). xrEngine PUBLIC-links xrLuabind/SDL2/xrCore/xrCDB/xrSound
# already, so those come along transitively for pch_script.h's lua.hpp/
# luabind headers. xrNetServer: GameObject.h itself needs
# DPNSEND_GUARANTEED (NET_Shared.h) as a default-parameter value - real,
# already-ported territory (notes section 12/13/18/19). xrParticles is NOT
# linked - zero #include from this batch's files, matching section 26's
# finding that xrGame -> xrParticles is genuinely zero real includes.
target_link_libraries(xrGame PUBLIC xrCore xrCDB xrSound xrEngine xrPhysics xrNetServer)
