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
  # Graph-engine-cycle-fix follow-up (integration pass, see notes file):
  # real recursive #include-closure verified, each compiles clean now
  # that the shared root cause (f91d6195 + this pass's further fixes to
  # graph_vertex.h/graph_abstract.h/alife_abstract_registry.h/
  # object_loader.h/ai_stalker.h) is resolved:
  alife_anomalous_zone.cpp
  alife_dynamic_object.cpp
  alife_object_registry.cpp
  alife_online_offline_group_brain.cpp
  alife_registry_container.cpp
  alife_simulator_base2.cpp
  alife_smart_terrain_task.cpp
  alife_storage_manager.cpp
  car_memory.cpp
  character_community.cpp
  character_rank.cpp
  character_reputation.cpp
  monster_community.cpp
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
  # Second ui/ batch (integration pass after the graph-engine-cycle fix,
  # f91d6195) - real recursive #include-closure verified, each compiles
  # clean:
  ui/ArtefactDetectorUI.cpp
  ui/UIAnimatedStatic.cpp
  ui/UIComboBox.cpp
  ui/UIDebugFonts.cpp
  ui/UIHint.cpp
  ui/UIKeyBinding.cpp
  ui/UIMessageBox.cpp
  ui/UIOptionsItem.cpp
  ui/UIOptionsManager.cpp
  ui/UIPropertiesBox.cpp
  ui/UIStatic.cpp
  # Third ui/ batch (this pass - see playground/xray-monolith-vulkan-port-notes.md,
  # the "ui/ sweep" milestone after section 31): 100 of the remaining 114
  # on-disk ui/*.cpp files, real per-file g++ -c verification, not just
  # closure-ranking. Systemic fixes this batch needed across most files:
  # the usual lowercase #include "stdafx.h" bug (section 28) on 49 of these
  # files; dozens of ordinary case-sensitivity/backslash #include fixes
  # (uiwindow.h->UIWindow.h, restrictions.h->Restrictions.h, ../actor.h->
  # ../Actor.h, ../level.h->../Level.h, etc. - see git diff for the full
  # list, not enumerated individually here); a real luabind API-drift gap
  # (luabind::policy::adopt is in a nested namespace - "using namespace
  # luabind;" alone doesn't pull in unqualified adopt<N>() - needed a
  # second "using namespace luabind::policy;" in 3 _script.cpp files); a
  # real pre-existing self-shadowing-typedef bug in xrCore/intrusive_ptr.h
  # (typedef object_type object_type;` - same bug class as the
  # graph_vertex.h/associative_vector_compare_predicate.h findings from
  # earlier passes, just never hit until a ui/ file finally instantiated
  # intrusive_ptr<T> for the first time in this port - fixed by renaming
  # the template parameter to _ip_object_type, keeping the exposed nested
  # type name unchanged); a missing win32_compat.h strncat_s (added
  # alongside the existing strncpy_s) and GetFullPathName (added, real
  # realpath()-backed implementation) stand-in; an unguarded <dinput.h> on
  # 13 files (wrapped in #ifdef _WIN32, matching the established
  # UIEditBox.cpp/UIMessageBoxEx.cpp precedent - all only ever used
  # win32_compat.h's portable DIK_* stand-ins, confirmed by grep per file);
  # and several narrow, real removals of now-nonexistent GameSpy/MP-only
  # types (ServerList.h/CServerList, UIChatWnd.h/CUIChatWnd, game_cl_mp,
  # game_cl_deathmatch, game_cl_capture_the_artefact, game_cl_teamdeathmatch,
  # login_manager.h - all confirmed absent anywhere in this source tree via
  # grep, not a case-sensitivity issue - each removal narrowly scoped to the
  # dead/unreachable call site, documented in-place in the affected file).
  # See the notes file for the full per-bug breakdown.
  ui/MMSound.cpp
  ui/TeamInfo.cpp
  ui/UI3tButton.cpp
  ui/UIAchievements.cpp
  ui/UIActorMenuInitialize.cpp
  ui/UIActorMenuInventory.cpp
  ui/UIActorMenuUpgrade.cpp
  ui/UIActorMenu_action.cpp
  ui/UIActorStateInfo.cpp
  ui/UIBoosterInfo.cpp
  ui/UIButton_script.cpp
  ui/UIBuyWeaponTab.cpp
  ui/UIBuyWndShared.cpp
  ui/UICDkey.cpp
  ui/UICellCustomItems.cpp
  ui/UICellItem.cpp
  ui/UICellItemFactory.cpp
  ui/UICharacterInfo.cpp
  ui/UIComboBox_script.cpp
  ui/UICustomSpin.cpp
  ui/UIDemoPlayControl.cpp
  ui/UIDoubleProgressBar.cpp
  ui/UIDragDropListEx.cpp
  ui/UIDragDropReferenceList.cpp
  ui/UIEditBox_script.cpp
  ui/UIGameLog.cpp
  ui/UIGameTutorial.cpp
  ui/UIGameTutorialSimpleItem.cpp
  ui/UIGameTutorialVideoItem.cpp
  ui/UIHelper.cpp
  ui/UIInvUpgrade.cpp
  ui/UIInvUpgradeInfo.cpp
  ui/UIInvUpgradeProperty.cpp
  ui/UIInventoryUpgradeWnd.cpp
  ui/UIInventoryUpgradeWnd_add.cpp
  ui/UIInventoryUtilities.cpp
  ui/UIItemInfo.cpp
  ui/UILine.cpp
  ui/UILines.cpp
  ui/UIListBoxItem.cpp
  ui/UIListItemServer.cpp
  ui/UILogsWnd.cpp
  ui/UIMMShniaga.cpp
  ui/UIMPChangeMapAdm.cpp
  ui/UIMPServerAdm.cpp
  ui/UIMainIngameWnd.cpp
  ui/UIMap.cpp
  ui/UIMapInfo.cpp
  ui/UIMapInfo_script.cpp
  ui/UIMapLegend.cpp
  ui/UIMapList.cpp
  ui/UIMapWnd2.cpp
  ui/UIMessageBox_script.cpp
  ui/UIMessagesWindow.cpp
  ui/UIMpItemsStoreWnd.cpp
  ui/UIMpTradeWnd_init.cpp
  ui/UIMpTradeWnd_items.cpp
  ui/UIMpTradeWnd_misc.cpp
  ui/UIMpTradeWnd_trade.cpp
  ui/UIMpTradeWnd_wpn.cpp
  ui/UINewsItemWnd.cpp
  ui/UIOptConCom.cpp
  ui/UIOptionsManagerScript.cpp
  ui/UIOutfitInfo.cpp
  ui/UIPdaKillMessage.cpp
  ui/UIPdaMsgListItem.cpp
  ui/UIPdaWnd.cpp
  ui/UIProgressBar_script.cpp
  ui/UIProgressShape.cpp
  ui/UIPropertiesBox_script.cpp
  ui/UIRadioButton.cpp
  ui/UIRankingWnd.cpp
  ui/UIRankingsCoC.cpp
  ui/UIScriptWnd.cpp
  ui/UISecondTaskWnd.cpp
  ui/UISpeechMenu.cpp
  ui/UISpinNum.cpp
  ui/UISpinText.cpp
  ui/UIStatic_script.cpp
  ui/UISubLine.cpp
  ui/UITabButton.cpp
  ui/UITabButtonMP.cpp
  ui/UITabControl.cpp
  ui/UITabControl_script.cpp
  ui/UITabControl_scroll.cpp
  ui/UITabScrollArrows.cpp
  ui/UITalkDialogWnd.cpp
  ui/UITalkWnd.cpp
  ui/UITaskWnd.cpp
  ui/UITextureMaster.cpp
  ui/UITrackBar.cpp
  ui/UIWindow.cpp
  ui/UIWindow_script.cpp
  ui/UIWndCallback.cpp
  ui/UIWpnParams.cpp
  ui/UIXmlInit.cpp
  ui/UI_IB_Static.cpp
  ui/map_hint.cpp
  ui/ui_af_params.cpp
  ui/uiscriptwnd_script2.cpp
)

# ui/ sweep (this pass): of the 114 files that were still unwired after
# section 31, 100 now compile clean and are wired in above. The remaining
# 14 are EXCLUDED, each with real, individually-diagnosed evidence - not
# a guess, and not the same root cause across the board:
#
#   UIActorMenu.cpp(6) UIActorMenuDeadBodySearch.cpp(6) UIActorMenuTrade.cpp(6)
#   UIHudStatesWnd.cpp(7)
#     - all four reach ai/monsters/basemonster/base_monster.h (via
#       ../ai/monsters/BaseMonster/base_monster.h, case-fixed to the real
#       on-disk "basemonster"), which itself has a real, unrelated bug (an
#       undeclared `OUT` macro at two call sites, `int Type` shadowing a
#       template parameter) that would need fixing INSIDE ai/ - explicitly
#       out of scope for this pass per the task boundary. Not touched.
#   UIListBox_script.cpp(13) UIScriptWnd_script.cpp(7) UIActorMenu_script.cpp(1)
#     - all three register a class_<Derived, bases<...>, WrapType> (a
#       ::luabind::wrap_base-derived wrapper enabling Lua-side virtual
#       overrides) for a UI class ultimately deriving from the noncopyable
#       CUIWindow hierarchy, with a plain .def(constructor<>()). This
#       vendored luabind fork's constructor<Class, Pointer, Signature>
#       machinery (luabind/detail/constructor.hpp) ends up trying to
#       construct the WrapType from a raw base-class pointer in a way GCC
#       rejects ("no matching function for call to
#       CWrapperBase<T>::CWrapperBase(pointer)") - root cause not fully
#       chased into class.hpp's constructor_registration/default_pointer
#       template plumbing; a real, non-trivial pre-existing luabind/GCC
#       incompatibility, not something to force a workaround for. Worth a
#       dedicated future pass - likely affects other not-yet-attempted
#       wrap_base-registered UI _script.cpp files too.
#   UIMapWndActions.cpp(22) UIMapWnd.cpp(14)
#     - both reach a template-instantiation cascade through
#       action_base_inline.h/action_planner.h/condition_state_inline.h/
#       graph_engine_inline.h/operator_abstract_inline.h/problem_solver.h
#       (CActionPlanner<CUIMapWnd,...>/CProblemSolver<...>/CGraphEngine -
#       the "action planner" template family) - similar shape to the
#       graph-engine-cycle bug fixed in an earlier pass (section 30) but a
#       different, unrelated template family/root cause, not chased this
#       pass. A plausible dedicated-pass target, analogous to how the
#       graph-engine cycle got its own session.
#   UIServerInfo.cpp(1)
#     - needs ximage.h/xmemfile.h (CxImage/CxMemFile, for decoding a
#       downloaded MP server's map-preview screenshot) from the cximage
#       3rd-party library, which isn't vendored into this build at all -
#       real usage (CxMemFile/CxImage constructed and used, not a dead
#       include), not droppable narrowly. Vendoring a whole new image
#       library is out of scope for one file; deferred.
#   UIMpTradeWnd.cpp(10) UISpawnWnd.cpp(16) UISkinSelector.cpp(20)
#     - all three are genuinely MP-only dialogs (trade confirm, spawn/map
#       select, skin selector) built around game_cl_mp - a type confirmed
#       (grep) absent anywhere in this source tree, same as the several
#       game_cl_mp/ServerList.h/UIChatWnd.h gaps fixed narrowly elsewhere
#       in this batch. Unlike those, game_cl_mp is used pervasively
#       throughout each of these three files' real logic (4-6 call sites
#       each, not 1-2 isolated null-checked branches) - gutting them would
#       mean rewriting most of the class, not a narrow removal. Deferred.
#
# (parenthesized number = real g++ -c error count as of this pass, not a
# complexity estimate). ~254 ui/ files remained unstarted before this
# pass (271 minus the 17 from section 30's tail); after this pass, only
# these 14 remain (100 ported + the pre-existing 38).

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
  # Second general batch (integration pass after the graph-engine-cycle
  # fix, f91d6195) - real recursive #include-closure verified, each
  # compiles clean:
  CustomMonster_VCPU.cpp
  HUDCrosshair.cpp
  PHSimpleCallsScript.cpp
  detail_path_manager_smooth.cpp
  ef_pattern.cpp
  movement_manager_physic.cpp
)

# UPDATE (see notes file, latest integration pass): the graph-engine
# instantiation cycle described below at length is now actually fixed
# (graph_vertex.h/graph_abstract.h restructured so CVertex takes
# _edge_weight_type as an explicit template parameter instead of deriving
# it via nested lookup - commit f91d6195 - plus several further real bugs
# in the same instantiation chain found once the cycle itself cleared:
# a second, genuinely-needed self-shadowing-typedef case in CVertex itself
# fixed by renaming the template parameter rather than the exposed nested
# type; a dead self-shadowing typedef in CGraphAbstract just deleted;
# alife_abstract_registry.h's `typedef typename _data_type _data;` -
# `typename` is illegal before a bare template parameter, only before a
# qualified dependent name; xrServerEntities/object_loader.h missing
# `typename` before `object_type_traits::remove_const<T1>::type`;
# ai_stalker.h's two illegal unscoped-enum forward declarations, replaced
# with real #includes; _rect.h's `in(Tvector&)` binding a temporary,
# fixed to `const Tvector&`; a `dynamic_cast`-casts-away-constness bug in
# Weapon.cpp from calling smart_cast<CActor*>/<CGameObject*> on H_Parent()
# from inside a const method). Of the 50 files originally excluded here,
# 36 now compile clean and are wired in below (Game3_ALife group above for
# the alife_*/character_*/car_memory/monster_community ones; a new
# "Game3_Vehicles_Weapons" group below for the Car*/Weapon*/Helicopter*/
# Grenade ones). The remaining 14 have real issues of their own, not yet
# individually chased - EXCLUDED, each needing its own future look:
#   HelicopterMovementManager.cpp(1) WeaponBinocularsVision.cpp(6)
#   WeaponMagazined.cpp(15) WeaponMagazinedWGrenade.cpp(2)
#   WeaponStatMgun.cpp(1) WeaponStatMgunFire.cpp(15) WeaponStatMgunMisc.cpp(1)
#   alife_monster_detail_path_manager.cpp(7) alife_monster_patrol_path_manager.cpp(1)
#   alife_spawn_registry.cpp(3) alife_spawn_registry_spawn.cpp(1)
#   alife_surge_manager.cpp(5) alife_trader_abstract.cpp(2) alife_update_manager.cpp(7)
# (parenthesized number = real g++ -c error count as of this pass, not a
# file size/complexity estimate - each is a concrete, findable bug or
# small set of bugs, same shape as everything fixed above, just not yet
# done). See the notes file's per-batch section for the exact original
# root-cause diagnosis this comment used to carry in full.

target_sources_grouped(
  TARGET xrGame
  NAME "Game3_Vehicles_Weapons"
  FILES
  Car.cpp
  CarCameras.cpp
  CarDrone.cpp
  CarInput.cpp
  CarNew.cpp
  CarWeapon.cpp
  Grenade.cpp
  Helicopter.cpp
  Helicopter2.cpp
  HelicopterWeapon.cpp
  Weapon.cpp
  WeaponAutomaticShotgun.cpp
  WeaponDispersion.cpp
  WeaponKnife.cpp
  WeaponPistol.cpp
  WeaponRG6.cpp
  WeaponRPG7.cpp
  WeaponRevolver.cpp
  WeaponSSRS.cpp
  WeaponShotgun.cpp
  WeaponStatMgunBarrel.cpp
  WeaponStatMgunIR.cpp
  weaponBM16.cpp
)

# ik/ - a small, genuinely self-contained inverse-kinematics solver
# (University of Pennsylvania IKAN library, vendored directly into this
# module rather than as a separate 3rd-party target). Real #include-grep
# confirmed zero cross-module hub dependencies: aint/eqn/eulersolver/
# jtlimits/limb/math3d/mathTrig/Dof7control only reference each other and
# StdAfx.h; only IKLimb.cpp/.h reach outside ik/ at all, and everything
# they reach was already ported (Include/xrRender/Kinematics.h -
# abstract interface; GameObject.h - header only, not GameObject.cpp;
# xrPhysics/MathUtils.h + matrix_utils.h - xrPhysics is done;
# ik_anim_state.h/pose_extrapolation.h/game_object_space.h - all
# already-ported or self-contained; PHDebug.h - pulls in already-ported
# xrEngine/StatGraph.h + xrPhysics/debug_output.h). All 9 files compile
# clean with only mechanical fixes: the systemic lowercase stdafx.h case
# bug (section 28) on 6 files, 3 more case-sensitivity #include fixes
# (Dof7control.h, Kinematics.h, GameObject.h, xrPhysics casing), and one
# missing #include (xrPhysics/MathUtils.h for phInfinity/XRPHYSICS_API in
# limb.cxx, same "MSVC whole-program name visibility" gap as
# ini_table_loader.h's object_type_traits, section 29/30).
target_sources_grouped(
  TARGET xrGame
  NAME "IK"
  FILES
  ik/aint.cxx
  ik/eqn.cxx
  ik/eulersolver.cxx
  ik/jtlimits.cxx
  ik/limb.cxx
  ik/math3d.cpp
  ik/mathTrig.cpp
  ik/Dof7control.cpp
  ik/IKLimb.cpp
)

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
# ai/ - first real batch (see playground/xray-monolith-vulkan-port-notes.md,
# the ai/monsters port). Previous agent's uncommitted first pass fixed the
# systemic lowercase-stdafx.h case bug across ~142 files and a real
# illegal-pure-virtual-with-body bug in monster_state_manager.h. This pass
# found and fixed a major NEW bug class affecting nearly the entire
# ai/monsters/states/ + per-monster-manager family: unqualified access to
# CState<T>/CMonsterStateManager<T> dependent-base members (object,
# add_state, get_state, select_state, current_substate, prev_substate,
# time_state_started, etc.) - GCC's real two-phase lookup requires
# this-> qualification that MSVC never needed. Fixed via an automated
# GCC-diagnostic-driven sweep (not hand-editing each of the ~2800 call
# sites individually), verified by full recompilation, with a second
# pass to catch corruption where the same bare names collided with
# unrelated local variables/parameters/members in non-CState-family code
# (see notes for the full bug catalog: pure-virtual-with-body,
# make_xrstr overload-visibility-order, friend-class-only-forward-decl,
# ctor-mem-initializer-list this-> illegality, and more).
target_sources_grouped(
  TARGET xrGame
  NAME "AI_Common"
  FILES
ai/ai_monsters_misc.cpp
ai/crow/ai_crow.cpp
ai/phantom/phantom.cpp
ai/weighted_random.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "AI_Stalker"
  FILES
ai/stalker/ai_stalker_events.cpp
ai/stalker/ai_stalker_feel.cpp
ai/stalker/ai_stalker_misc.cpp
ai/stalker/ai_stalker_script_entity.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "AI_Trader"
  FILES
ai/trader/ai_trader.cpp
ai/trader/ai_trader_script.cpp
ai/trader/trader_animation.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "AI_Monsters_Core"
  FILES
ai/monsters/ai_monster_bones.cpp
ai/monsters/ai_monster_effector.cpp
ai/monsters/ai_monster_motion_stats.cpp
ai/monsters/ai_monster_squad.cpp
ai/monsters/ai_monster_squad_attack.cpp
ai/monsters/ai_monster_squad_manager.cpp
ai/monsters/ai_monster_squad_rest.cpp
ai/monsters/ai_monster_utils.cpp
ai/monsters/anim_triple.cpp
ai/monsters/anomaly_detector.cpp
ai/monsters/anti_aim_ability.cpp
ai/monsters/basemonster/base_monster.cpp
ai/monsters/basemonster/base_monster_anim.cpp
ai/monsters/basemonster/base_monster_feel.cpp
ai/monsters/basemonster/base_monster_misc.cpp
ai/monsters/basemonster/base_monster_net.cpp
ai/monsters/basemonster/base_monster_path.cpp
ai/monsters/basemonster/base_monster_startup.cpp
ai/monsters/basemonster/base_monster_think.cpp
ai/monsters/control_animation.cpp
ai/monsters/control_animation_base.cpp
ai/monsters/control_animation_base_accel.cpp
ai/monsters/control_animation_base_load.cpp
ai/monsters/control_animation_base_update.cpp
ai/monsters/control_critical_wound.cpp
ai/monsters/control_direction.cpp
ai/monsters/control_direction_base.cpp
ai/monsters/control_jump.cpp
ai/monsters/control_manager.cpp
ai/monsters/control_manager_custom.cpp
ai/monsters/control_melee_jump.cpp
ai/monsters/control_movement.cpp
ai/monsters/control_movement_base.cpp
ai/monsters/control_path_builder_base_set.cpp
ai/monsters/control_path_builder_base_update.cpp
ai/monsters/control_rotation_jump.cpp
ai/monsters/control_run_attack.cpp
ai/monsters/control_sequencer.cpp
ai/monsters/control_threaten.cpp
ai/monsters/controlled_actor.cpp
ai/monsters/corpse_cover.cpp
ai/monsters/energy_holder.cpp
ai/monsters/invisibility.cpp
ai/monsters/melee_checker.cpp
ai/monsters/monster_aura.cpp
ai/monsters/monster_corpse_manager.cpp
ai/monsters/monster_corpse_memory.cpp
ai/monsters/monster_cover_manager.cpp
ai/monsters/monster_enemy_manager.cpp
ai/monsters/monster_enemy_memory.cpp
ai/monsters/monster_event_manager.cpp
ai/monsters/monster_hit_memory.cpp
ai/monsters/monster_home.cpp
ai/monsters/monster_morale.cpp
ai/monsters/monster_sound_memory.cpp
ai/monsters/psy_aura.cpp
ai/monsters/telekinesis.cpp
ai/monsters/telekinetic_object.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "AI_Monsters_Creatures"
  FILES
ai/monsters/bloodsucker/bloodsucker.cpp
ai/monsters/bloodsucker/bloodsucker_alien.cpp
ai/monsters/bloodsucker/bloodsucker_script.cpp
ai/monsters/bloodsucker/bloodsucker_state_manager.cpp
ai/monsters/bloodsucker/bloodsucker_vampire_effector.cpp
ai/monsters/boar/boar.cpp
ai/monsters/boar/boar_script.cpp
ai/monsters/boar/boar_state_manager.cpp
ai/monsters/burer/burer.cpp
ai/monsters/burer/burer_fast_gravi.cpp
ai/monsters/burer/burer_script.cpp
ai/monsters/burer/burer_state_manager.cpp
ai/monsters/cat/cat.cpp
ai/monsters/cat/cat_script.cpp
ai/monsters/cat/cat_state_manager.cpp
ai/monsters/chimera/chimera.cpp
ai/monsters/chimera/chimera_script.cpp
ai/monsters/chimera/chimera_state_manager.cpp
ai/monsters/controller/controller.cpp
ai/monsters/controller/controller_animation.cpp
ai/monsters/controller/controller_direction.cpp
ai/monsters/controller/controller_psy_hit.cpp
ai/monsters/controller/controller_psy_hit_effector.cpp
ai/monsters/controller/controller_script.cpp
ai/monsters/controller/controller_state_manager.cpp
ai/monsters/dog/dog.cpp
ai/monsters/dog/dog_script.cpp
ai/monsters/dog/dog_state_manager.cpp
ai/monsters/flesh/flesh.cpp
ai/monsters/flesh/flesh_script.cpp
ai/monsters/flesh/flesh_state_manager.cpp
ai/monsters/fracture/fracture.cpp
ai/monsters/fracture/fracture_script.cpp
ai/monsters/fracture/fracture_state_manager.cpp
ai/monsters/poltergeist/poltergeist.cpp
ai/monsters/poltergeist/poltergeist_ability.cpp
ai/monsters/poltergeist/poltergeist_flame_thrower.cpp
ai/monsters/poltergeist/poltergeist_movement.cpp
ai/monsters/poltergeist/poltergeist_script.cpp
ai/monsters/poltergeist/poltergeist_state_manager.cpp
ai/monsters/poltergeist/poltergeist_telekinesis.cpp
ai/monsters/pseudodog/pseudodog.cpp
ai/monsters/pseudodog/pseudodog_psi_effector.cpp
ai/monsters/pseudodog/pseudodog_script.cpp
ai/monsters/pseudodog/pseudodog_state_manager.cpp
ai/monsters/pseudodog/psy_dog.cpp
ai/monsters/pseudodog/psy_dog_aura.cpp
ai/monsters/pseudodog/psy_dog_state_manager.cpp
ai/monsters/pseudogigant/pseudo_gigant.cpp
ai/monsters/pseudogigant/pseudo_gigant_step_effector.cpp
ai/monsters/pseudogigant/pseudogigant_script.cpp
ai/monsters/pseudogigant/pseudogigant_state_manager.cpp
ai/monsters/rats/ai_rat.cpp
ai/monsters/rats/ai_rat_animations.cpp
ai/monsters/rats/ai_rat_behaviour.cpp
ai/monsters/rats/ai_rat_feel.cpp
ai/monsters/rats/ai_rat_fire.cpp
ai/monsters/rats/ai_rat_templates.cpp
ai/monsters/rats/rat_state_activation.cpp
ai/monsters/rats/rat_state_initialize.cpp
ai/monsters/rats/rat_state_switch.cpp
ai/monsters/snork/snork.cpp
ai/monsters/snork/snork_jump.cpp
ai/monsters/snork/snork_script.cpp
ai/monsters/snork/snork_state_manager.cpp
ai/monsters/tushkano/tushkano.cpp
ai/monsters/tushkano/tushkano_script.cpp
ai/monsters/tushkano/tushkano_state_manager.cpp
ai/monsters/zombie/zombie.cpp
ai/monsters/zombie/zombie_script.cpp
ai/monsters/zombie/zombie_state_manager.cpp
)

# ai/ still excluded (this pass) - 11 of 151 ai/*.cpp files, real per-file
# g++ -c error counts, honestly not yet chased further:
#
# 9 of these 11 all cascade from ONE still-genuinely-unresolved root cause
# in the shared A*/Dijkstra graph-pathfinding template library (a_star.h/
# dijkstra.h/vertex_manager_fixed.h/vertex_manager_hash_fixed.h/
# data_storage_*.h/builder_allocator_constructor.h/
# manager_builder_allocator_constructor.h) - this pass found and fixed
# MANY real self-shadowing-typedef bugs in that exact family (notes
# section 30b's bug class, recurring in operator_condition.h, a_star.h
# x2, vertex_manager_hash_fixed.h, vertex_manager_fixed.h, dijkstra.h,
# problem_solver.h, operator_abstract.h, action_planner.h) plus several
# missing `::template` disambiguators before dependent nested templates
# (a_star.h, edge_path.h, vertex_manager_*.h, data_storage_bucket_list.h/
# data_storage_double_linked_list.h/data_storage_single_linked_list.h) -
# this genuinely unblocked most of the "action planner" second template
# cycle the standing task brief flagged (CActionBase/CActionPlanner/
# CProblemSolver via action_base_inline.h/action_planner.h/
# condition_state_inline.h/graph_engine_inline.h/operator_abstract_inline.h/
# problem_solver.h - all now compile). The one remaining root cause: one of
# the vertex-manager family's `CDataStorage` template parameters defaults
# to a bare `CBuilderAllocatorConstructor` (a 2-parameter template) with no
# template arguments at all - not a lookup/shadowing bug, a genuinely
# incomplete default that was apparently never actually relied upon by any
# instantiation MSVC needed until this exact call chain. Fixing it needs
# knowing what path-builder/vertex-allocator types the default *should*
# supply, which isn't inferable from the surrounding code - guessing would
# risk a wrong (silently different) default rather than a compile fix, so
# left honestly unresolved rather than forced:
#   basemonster/base_monster_script.cpp(1) control_path_builder.cpp(1)
#   control_path_builder_base.cpp(1) control_path_builder_base_path.cpp(1)
#   stalker/ai_stalker_cover.cpp(1) stalker/ai_stalker_script.cpp(1)
#   basemonster/base_monster_debug.cpp(2) stalker/ai_stalker.cpp(2)
#   stalker/ai_stalker_fire.cpp(2)
# (parenthesized number = real g++ -c error count once past the shared
# root cause above - each file's own single extra error is a different,
# not-yet-chased issue: a `luabind::type_id` used as if it were a pointer
# in ai_stalker.cpp, `get_operator` needing a `this->`/qualification fix
# in ai_stalker_fire.cpp, etc.)
#
# 2 more are unrelated to the above and have their own real, not-yet-
# chased issues:
#   monsters/rats/ai_rat_fsm.cpp(42) - CAI_Rat member/method names this
#     file references (m_thinking, vfUpdateSpawnPosition, aiRatDie,
#     vfSetFire, ...) don't exist anywhere on the current CAI_Rat/rat
#     state-manager classes - looks like this file targets an older/
#     different version of the class's own API, not a mechanical port
#     bug; needs a real per-symbol audit against what CAI_Rat actually
#     exposes today.
#   stalker/ai_stalker_debug.cpp(46) - hits the same shared graph-engine
#     root cause above, plus its own independent issues (missing
#     `psAI_Flags` extern, several `CDebugRenderer::draw_line`/
#     `draw_aabb`/`draw_ellipse` calls missing the `bool bHud` parameter
#     added since this file was last touched - same shape as the fix
#     already applied to rats/ai_rat_templates.cpp).
target_link_libraries(xrGame PUBLIC xrCore xrCDB xrSound xrEngine xrPhysics xrNetServer)
