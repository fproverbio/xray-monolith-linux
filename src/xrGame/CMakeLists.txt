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

# Integration pass (see playground/xray-monolith-vulkan-port-notes.md, next
# numbered section after 28) - candidate batch reconstructed from four
# parallel agents' uncommitted work after a simultaneous session-limit cutoff.
# Added here as one big candidate pool, verified via real full-tree rebuilds,
# with genuinely-blocked files pulled back out below (see exclusion comments
# after this block). See the notes section for the full per-cluster story.
target_sources_grouped(
  TARGET xrGame
  NAME "Game3_Vehicles_Weapons"
  FILES
  CameraFirstEye.cpp
  CarDamageParticles.cpp
  CarDoors.cpp
  CarExhaust.cpp
  CarLights.cpp
  CarSound.cpp
  CarWheels.cpp
  GrenadeLauncher.cpp
  Silencer.cpp
  Tracer.cpp
  WeaponAmmo.cpp
  WeaponBinoculars.cpp
  WeaponBinoculars_script.cpp
  WeaponCustomAuto.cpp
  WeaponCustomPistol.cpp
  WeaponGroza.cpp
  WeaponMagazineExtended.cpp
  WeaponSVD.cpp
  WeaponSVU.cpp
  WeaponUSP45.cpp
  WeaponUpgrade.cpp
  WeaponVal.cpp
  WeaponVintorez.cpp
  WeaponWalther.cpp
  weaponBM16_script.cpp
  weapon_ammo_dump_impl.cpp
  weapon_dump_impl.cpp
  Wound.cpp
  PHCommander.cpp
  PHMovementDynamicActivate.cpp
  PHSimpleCalls.cpp
  PHSoundPlayer.cpp
  ParticlesObject.cpp
  PostprocessAnimator.cpp
  EffectorFall.cpp
  FryupZone.cpp
  GlobalFeelTouch.cpp
  Message_Filter.cpp
  UIStaticItem.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "Game3_ALife"
  FILES
  alife_combat_manager.cpp
  alife_communication_manager.cpp
  alife_creature_abstract.cpp
  alife_graph_registry.cpp
  alife_group_abstract.cpp
  alife_group_registry.cpp
  alife_human_abstract.cpp
  alife_human_object_handler.cpp
  alife_interaction_manager.cpp
  alife_monster_abstract.cpp
  alife_monster_base.cpp
  alife_monster_movement_manager.cpp
  alife_object.cpp
  alife_online_offline_group.cpp
  alife_schedule_registry.cpp
  alife_simulator_header.cpp
  alife_smart_terrain_registry.cpp
  alife_smart_zone.cpp
  alife_spawn_registry_header.cpp
  alife_story_registry.cpp
  alife_switch_manager.cpp
  alife_time_manager.cpp
  alife_trader.cpp
  relation_registry_fights.cpp
  rat_state_base.cpp
  rat_state_manager.cpp
  restricted_object_obstacle.cpp
  space_restriction_base.cpp
  seniority_hierarchy_holder.cpp
  squad_hierarchy_holder.cpp
  team_hierarchy_holder.cpp
  stalker_velocity_collection.cpp
  stalker_velocity_holder.cpp
  steering_behaviour.cpp
  traffic_optimization.cpp
  danger_cover_location.cpp
  level_graph_vertex.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "Game3_UI"
  FILES
  ui/Restrictions.cpp
  ui/UIBtnHint.cpp
  ui/UIButton.cpp
  ui/UICarPanel.cpp
  ui/UICheckButton.cpp
  ui/UICustomEdit.cpp
  ui/UIDialogWnd.cpp
  ui/UIEditBox.cpp
  ui/UIEditBoxEx.cpp
  ui/UIEditKeyBind.cpp
  ui/UIFixedScrollBar.cpp
  ui/UIFrameLineWnd.cpp
  ui/UIFrameWindow.cpp
  ui/UILanimController.cpp
  ui/UIListBox.cpp
  ui/UIListBoxItemMsgChain.cpp
  ui/UIMessageBoxEx.cpp
  ui/UIMoneyIndicator.cpp
  ui/UIProgressBar.cpp
  ui/UIRankIndicator.cpp
  ui/UIScrollBar.cpp
  ui/UIScrollBox.cpp
  ui/UIScrollView.cpp
  ui/UIStatix.cpp
  ui/UITrackButton.cpp
  ui/ui_arrow.cpp
  ui/xrUIXmlParser.cpp
  ui_base.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "Game3_General"
  FILES
  configs_common.cpp
  console_registrator_script.cpp
  damage_manager.cpp
  debug_renderer.cpp
  ef_storage.cpp
  filereceiver_node.cpp
  gsc_dsigned_ltx.cpp
  hit_immunity.cpp
  ik_anim_state.cpp
  ik_limb_state.cpp
  ik_object_shift.cpp
  imotion_position.cpp
  imotion_velocity.cpp
  interactive_animation.cpp
  magic_minimize_1d.cpp
  min_obb.cpp
  particle_params.cpp
  pose_extrapolation.cpp
  pp_effector_distance.cpp
  screenshots_common.cpp
  screenshots_writer.cpp
  script_abstract_action.cpp
  script_action_condition.cpp
  script_animation_action.cpp
  script_hit.cpp
  script_object.cpp
  script_particles.cpp
  sound_collection_storage.cpp
  spectator_camera_first_eye.cpp
  xr_dsa_signer.cpp
  xr_dsa_verifyer.cpp
  xr_level_controller.cpp
)

# EXCLUDED - integration pass (see playground/xray-monolith-vulkan-port-notes.md,
# section 29): 50 files from the Weapon*/Car*/Helicopter* and alife_*/
# character_*/monster_community clusters, all blocked by the exact same real,
# confirmed root cause - a genuine mutual-recursive class-template
# instantiation cycle in the graph-pathfinding engine
# (CGraphAbstract/CVertex/CEdge, graph_abstract.h/graph_vertex.h/graph_edge.h):
# CGraphAbstract::EDGES needs CVertex complete; CVertex::_edge_weight_type
# needs CEdge (nested in _graph_type::CEdge) complete; CEdge's own CEdgeBase
# base needs CVertex::_vertex_id_type - a request for a member of the *exact
# CVertex specialization already being instantiated*, looping back in through
# a different template mid-instantiation. GCC treats a class template as
# wholly "incomplete" to any external nested-name lookup for its entire
# instantiation (even for a member declared/processed earlier in the same
# pass); the original MSVC target tolerated this, standard C++ does not
# provably require compilers to. See graph_vertex.h's own inline comment (on
# the `typedef typename _edge_type::_edge_weight_type _edge_weight_type;`
# line) for the full trace - reproduced via a real minimal standalone
# instantiation test (CGraphAbstract<CPatrolPoint, float, unsigned int,
# xr_empty>), not guessed at. A real fix needs restructuring this three-way
# dependency (e.g. an explicit _edge_weight_type template parameter on
# CVertex instead of deriving it via nested lookup through CEdge) - a
# genuine, non-mechanical redesign of shared graph-engine code touching
# dijkstra.h/a_star.h/graph_engine.h and everything built on them, out of
# scope for a mechanical port pass. Every file below reaches this same
# instantiation somewhere in its own #include closure (CGraphAbstract
# instantiated for CPatrolPoint - patrol paths; CPhrase* - dialogue graphs,
# reached transitively through PhraseDialog.h from Weapon.h's own hud/hit
# item-holder chain; or CServerEntityWrapper* - the ALife registry/spawn
# graph) - none of these 50 files have their own separate bug beyond this
# one shared root cause (re-verified individually, not assumed):
#   Car.cpp CarCameras.cpp CarDrone.cpp CarInput.cpp CarNew.cpp CarWeapon.cpp
#   Grenade.cpp Helicopter.cpp Helicopter2.cpp HelicopterMovementManager.cpp
#   HelicopterWeapon.cpp Weapon.cpp WeaponAutomaticShotgun.cpp
#   WeaponBinocularsVision.cpp WeaponDispersion.cpp WeaponKnife.cpp
#   WeaponMagazined.cpp WeaponMagazinedWGrenade.cpp WeaponPistol.cpp
#   WeaponRG6.cpp WeaponRPG7.cpp WeaponRevolver.cpp WeaponSSRS.cpp
#   WeaponShotgun.cpp WeaponStatMgun.cpp WeaponStatMgunBarrel.cpp
#   WeaponStatMgunFire.cpp WeaponStatMgunIR.cpp WeaponStatMgunMisc.cpp
#   alife_anomalous_zone.cpp alife_dynamic_object.cpp
#   alife_monster_detail_path_manager.cpp alife_monster_patrol_path_manager.cpp
#   alife_object_registry.cpp alife_online_offline_group_brain.cpp
#   alife_registry_container.cpp alife_simulator_base2.cpp
#   alife_smart_terrain_task.cpp alife_spawn_registry.cpp
#   alife_spawn_registry_spawn.cpp alife_storage_manager.cpp
#   alife_surge_manager.cpp alife_trader_abstract.cpp alife_update_manager.cpp
#   car_memory.cpp character_community.cpp character_rank.cpp
#   character_reputation.cpp monster_community.cpp weaponBM16.cpp
# All 50 files are otherwise real, unmodified-since-revert-worthy source
# (their own genuine bugs this pass found and fixed - case sensitivity,
# missing typename, dependent-base lookup, etc. - are still fixed in place;
# they're excluded here purely on this one shared blocker) - a future
# dedicated pass fixing the graph engine's instantiation order should be
# able to add all 50 back with zero further work on their own account.

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
