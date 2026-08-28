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

# Follow-up pass (see notes file, milestone after section 33): the `OUT`
# macro bug in ai/monsters/basemonster/base_monster.h was fixed for real
# during the ai/ sweep (added to xrCore/win32_compat.h) - re-checked and
# all four of these now compile clean:
target_sources_grouped(
  TARGET xrGame
  NAME "Game3_UI"
  FILES
  ui/UIActorMenu.cpp
  ui/UIActorMenuDeadBodySearch.cpp
  ui/UIActorMenuTrade.cpp
  ui/UIHudStatesWnd.cpp
)

# Follow-up pass: UIMapWndActions.cpp/UIMapWnd.cpp's shared
# path_manager_solver_inline.h blocker is fixed for real now - see the
# "AI_PathBuilder_LevelCascade" comment further down (near
# ai/monsters/control_path_builder.cpp) for the full chain of fixes; that
# same cascade covers both files, exactly as predicted when this exclusion
# was first written.
target_sources_grouped(
  TARGET xrGame
  NAME "Game3_UI_MapWnd"
  FILES
  ui/UIMapWndActions.cpp
  ui/UIMapWnd.cpp
)

# ui/ cleanup pass: the 3 "wrap_base" exclusions (see the comment further
# down near their old exclusion entry for the full root-cause diagnosis)
# were a real, narrow HolderType-vs-WrapperType class_<> argument-slot
# mixup, not a deep luabind/GCC incompatibility - fixed and wired in.
target_sources_grouped(
  TARGET xrGame
  NAME "Game3_UI_Wrappers"
  FILES
  ui/UIActorMenu_script.cpp
  ui/UIListBox_script.cpp
  ui/UIScriptWnd_script.cpp
)

# ui/ sweep (this pass): of the 114 files that were still unwired after
# section 31, 100 now compile clean and are wired in above.
#
# UPDATE (ui/ cleanup pass): the "3 wrap_base files" above turned out to
# be a real, narrow, fully-diagnosed bug, not a deep luabind/GCC
# incompatibility - see luabind/class.hpp's own forward declaration:
#   template<typename T, typename BaseOrBases = no_bases,
#            typename HolderType = null_type, typename WrapperType = null_type>
# The 3rd class_<> template argument is HolderType, NOT the Lua-override
# WrapperType (that's the 4th). All three files passed their *Wrapper
# class positionally as the 3rd argument (`class_<T, Base, WrapType>`),
# which silently compiles right up until a constructor is also
# registered - at which point the fork's constructor-registration
# machinery tries to construct "HolderType" (really the Wrapper class)
# from a raw pointer, which doesn't exist ("no matching function for
# call to ...Wrapper::Wrapper(...pointer)"). UIActorMenu_script.cpp had a
# related-but-distinct bug: CUIActorMenu genuinely has two base classes
# (CUIDialogWnd + CUIWndCallback) passed positionally instead of wrapped
# in `bases<>`, so the second base landed in the HolderType slot instead.
# All three fixed (explicit `null_type` in the HolderType slot + moving
# the Wrapper class to the 4th slot; `bases<>` for the two-base case) and
# wired in below as "Game3_UI_Wrappers". UIScriptWnd_script.cpp also
# needed one more real fix once the above unblocked it: `call_member` in
# uiscriptwnd_script.h needed `luabind::` qualification (its own `using
# namespace luabind` doesn't reach this shared header from every includer).
#
# The remaining 5 are OUT OF SCOPE by deliberate project decision, not a
# TODO - this port targets singleplayer only, no functional multiplayer,
# and all 5 are MP-only:
#
#   UIServerInfo.cpp
#     - an MP server-browser widget (decodes a downloaded server's
#       map-preview screenshot via CxImage/CxMemFile, from the
#       un-vendored cximage 3rd-party library). Confirmed via grep that
#       CUIServerInfo is referenced nowhere else in the tree - genuinely
#       dead code in a singleplayer-only build, not merely hard to port.
#   UIMpTradeWnd.cpp UISpawnWnd.cpp UISkinSelector.cpp
#     - genuinely MP-only dialogs (trade confirm, spawn/map select, skin
#       selector) built pervasively around game_cl_mp, a type confirmed
#       absent anywhere in this source tree (the whole MP/GameSpy
#       subsystem was removed early in this port, see commit
#       1d4ec53e). Fixing these to compile without game_cl_mp would mean
#       either resurrecting real MP networking or gutting each class to
#       an inert stub for no functional benefit - not worth doing for a
#       singleplayer-only port.
#
# ~254 ui/ files remained unstarted before the original ui/ sweep (271
# minus the 17 from section 30's tail); after that pass, 14 remained - a
# follow-up pass fixed 4 fully (moved to Game3_UI), a later pass (see
# "Game3_UI_MapWnd" above) fixed UIMapWndActions.cpp/UIMapWnd.cpp for
# real too, and the cleanup pass above fixed the 3 wrap_base files. `ui/`
# is DONE: 147/152, the remaining 5 out of scope by design.

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
# Grenade ones). UPDATE (xrGame root sweep, Game4_RootBatch3): 5 more of
# the original 14 - HelicopterMovementManager.cpp,
# alife_monster_detail_path_manager.cpp, alife_monster_patrol_path_manager.cpp,
# alife_spawn_registry.cpp, alife_spawn_registry_spawn.cpp - are now fixed
# too, as a side effect of that pass's dependent-base/typename fixes rather
# than individually targeted. UPDATE (Game4_RootBatch4): alife_update_manager.cpp
# is fixed too (path_manager_game_level_inline.h's this-> fixes). The
# remaining 8 still have real issues of their own, not yet individually
# chased - EXCLUDED, each needing its own future look:
#   WeaponBinocularsVision.cpp(6)
#   WeaponMagazined.cpp(15) WeaponMagazinedWGrenade.cpp(2)
#   WeaponStatMgun.cpp(1) WeaponStatMgunFire.cpp(15) WeaponStatMgunMisc.cpp(1)
#   alife_surge_manager.cpp(5) alife_trader_abstract.cpp(2)
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

# Follow-up pass (see notes file): the vertex-manager root cause below
# is now genuinely FIXED (not just diagnosed) - `_data_storage`'s
# ill-formed default `= CBuilderAllocatorConstructor` (a bare,
# unspecialized class template used where a type is required) in both
# vertex_manager_fixed.h and vertex_manager_hash_fixed.h was replaced
# with `= void` (a syntactically valid placeholder, confirmed via
# exhaustive grep to never actually be substituted - every real call
# site supplies `_data_storage` explicitly via the
# CFixedVertexManager/CHashFixedVertexManager macros). This alone, plus
# a further chain of ~10 real bugs surfaced once it cleared (two illegal
# in-class explicit specializations of member templates in
# problem_solver.h rewritten as `if constexpr`, more self-shadowing
# parameters, several missing `typename`/`this->`/`inherited::`
# qualifications across problem_solver_inline.h/action_planner_inline.h/
# condition_state_inline.h/operator_abstract_inline.h), unblocked 6 of
# the 9 files below for real:
target_sources_grouped(
  TARGET xrGame
  NAME "AI_PathBuilder_Fixed"
  FILES
  ai/monsters/basemonster/base_monster_script.cpp
  ai/monsters/control_path_builder_base.cpp
  ai/monsters/control_path_builder_base_path.cpp
  ai/stalker/ai_stalker_cover.cpp
  ai/stalker/ai_stalker_script.cpp
  ai/stalker/ai_stalker_fire.cpp
)

# Follow-up pass (see notes file): 3 more of the remaining 5 fixed for
# real -
#   basemonster/base_monster_debug.cpp: `psAI_Flags` genuinely IS a real
#     global (ai_debug.h:48's `extern Flags32 psAI_Flags;` inside
#     `#ifndef MASTER_GOLD`, which is dead in every TU of this build due
#     to a pre-existing xrCore.h ordering quirk - see
#     alife_level_registry_inline.h's comment for the full explanation)
#     - fixed the same way as everywhere else: a local `extern Flags32
#     psAI_Flags;` declaration.
#   stalker/ai_stalker.cpp: `message.info()->name()` - `luabind::
#     cast_failed::info()` returns `type_id` BY VALUE with `.name()` a
#     direct member (not a pointer) - `->` should have been `.`.
#   stalker/ai_stalker_debug.cpp: same `psAI_Flags` fix, PLUS a genuine
#     MSVC-permissive-extension bug (a `u32 j` declared inside a
#     `for (u32 i=0, j=0; ...)` init-statement, then used again AFTER
#     the loop closes - illegal in standard C++, for-init variables are
#     scoped to the loop only; MSVC's old pre-2005 compat mode let this
#     slide) fixed by hoisting `j`'s declaration above the loop, PLUS
#     ~39 `CDebugRenderer::draw_line`/`draw_aabb`/`draw_ellipse`/
#     `draw_obb` calls missing the `bool bHud` parameter (no default
#     value - genuinely required, not optional) added since this file
#     was last touched, PLUS two missing `typename`s in a template
#     function (`draw_planner`) never previously instantiated with this
#     particular `planner_type`, PLUS a matching `this->` dependent-base
#     qualification fix in the unrelated shared `game_path_manager_inline.h`
#     (`path()`/`m_intermediate_index` reached through `inherited`).
target_sources_grouped(
  TARGET xrGame
  NAME "AI_PathBuilder_Fixed2"
  FILES
  ai/monsters/basemonster/base_monster_debug.cpp
  ai/stalker/ai_stalker.cpp
  ai/stalker/ai_stalker_debug.cpp
)

# Follow-up pass: the "level path manager" cascade flagged above turned out
# smaller than feared - only 5 files, not ~15 - once actually chased:
# path_manager_level_nearest_vertex_inline.h, path_manager_level_inline.h,
# data_storage_bucket_list_inline.h, a_star_inline.h, and
# data_storage_double_linked_list_inline.h, all the same missing-`this->`-
# for-dependent-base-member pattern as every other fix in this project.
# That alone got control_path_builder.cpp compiling. Chasing
# UIMapWndActions.cpp/UIMapWnd.cpp's shared path_manager_solver_inline.h
# blocker further surfaced 4 more distinct, unrelated bugs in the same
# family: (1) `this->`-qualification again in path_manager_solver_inline.h
# itself (graph/data_storage/start_node_index/goal_node_index/
# max_visited_node_count/max_range/max_iteration_count/best_node_index, all
# from the CPathManagerGeneric dependent base); (2) `typename` illegally
# applied to `_Graph::reverse_search`, which is an enum VALUE (see
# CProblemSolver's anonymous enum) not a type - the bare `typename` there
# was always ill-formed; (3) a local variable literally named `_vertex` in
# vertex_manager_hash_fixed_inline.h::create_vertex() shadowing the
# enclosing CDataStorage template's own `_vertex` template-template-
# parameter (GCC's -Wtemplate-body escalates this shadow to a hard
# instantiation failure) - renamed the local to `found_vertex`; (4) two
# missing `typename`s in edge_path_inline.h's `xr_vector<_edge_type>::
# reverse_iterator`/`::iterator`. Finally, `hash_fixed_vertex_manager::
# to_u32(GraphEngineSpace::CWorldState const&)` (needed transitively by
# vertex_manager_hash_fixed_inline.h's hash_index()) was declared far below
# its own qualified call site inside graph_engine.h - GCC's -Wtemplate-body
# checks a qualified name's namespace membership at the *parse* point of
# the calling template, not deferred to instantiation - fixed by moving the
# whole `#ifndef AI_COMPILER` condition_state.h/operator_condition.h/
# operator_abstract.h include block plus the to_u32() overload itself ahead
# of vertex_manager_hash_fixed.h's own #include in graph_engine.h (verified
# AI_COMPILER is never defined anywhere in this build, so this is a pure
# reorder, not a behavior change). ai/ is now 151/151 - fully done.
target_sources_grouped(
  TARGET xrGame
  NAME "AI_PathBuilder_LevelCascade"
  FILES
  ai/monsters/control_path_builder.cpp
)

# monsters/rats/ai_rat_fsm.cpp - the ~9 "missing" CAI_Rat identifiers
# (m_thinking, vfUpdateSpawnPosition, aiRatDie, vfSetFire, vfComputeNewPosition,
# vfSetMovementType, m_tSafeSpawnPosition, aiRatAttackFire, aiRatAttackRun)
# turned out to be a real, fully-traceable rename mismatch, not a genuine API
# gap: confirmed via grep that none of them are referenced anywhere else in
# the tree, and every one maps 1:1 onto an existing CAI_Rat member once the
# file's own inline comments and sibling .cpp definitions are cross-checked
# (aiRatDie -> aiRatDeath; aiRatAttackFire/aiRatAttackRun -> aiRatAttackRange/
# aiRatAttackMelee, per this file's own `//aiRatAttackFire`/`//aiRatAttackRun`
# comments directly above the `case aiRatAttackRange/aiRatAttackMelee:`
# labels that call AttackFire()/AttackRun(); vfComputeNewPosition(bool,bool)
# -> move(bool,bool), matching signature and defaults exactly, defined in
# ai_rat_templates.cpp; vfSetFire -> fire(), vfSetMovementType -> movement_type(),
# both also in ai_rat_templates.cpp; vfUpdateSpawnPosition -> update_home_position(),
# defined in ai_rat_behaviour.cpp; m_tSafeSpawnPosition -> m_home_position,
# consistent with ai_rat_behaviour.cpp's own use of m_home_position as the
# squad's shared "safe" reference point distinct from the wandering-goal
# m_tSpawnPosition). m_thinking itself is a genuinely new member (only ever
# set, never read, purely a Think()-loop reentrancy flag) - added to
# ai_rat.h next to the closely-related m_bStopThinking/m_bStateChanged.
target_sources_grouped(
  TARGET xrGame
  NAME "AI_Rat_FSM"
  FILES
  ai/monsters/rats/ai_rat_fsm.cpp
)

# xrGame/ root sweep, first batch: 172 files that compile clean with zero
# changes needed once the systemic lowercase/mis-cased-#include bug (the
# same class as the already-fixed ~686-file stdafx.h issue, but here
# affecting real content headers like level.h/gameobject.h/actor.h/
# inventory.h/weapon.h/xrserver.h/etc.) was fixed project-wide via a
# script-driven, case-insensitive #include resolution pass across all of
# xrGame (1104 individual #include directives corrected across 349 files
# total, in root/ + ai/ + ui/ - only the files listed below were actually
# gated on nothing else and compile clean here; the many others whose
# includes were also fixed already were compiling for other reasons or
# remain blocked by real, separate issues untouched by this pass).
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch1"
  FILES
  ActorCondition.cpp
  ActorEffector_script.cpp
  BottleItem_script.cpp
  CarScript.cpp
  CustomOutfit_script.cpp
  CustomZone_script.cpp
  DestroyablePhysicsObject.cpp
  ExplosiveScript.cpp
  F1.cpp
  FoodItem_script.cpp
  GameTask.cpp
  GameTask_script.cpp
  GametaskManager.cpp
  HairsZone.cpp
  HairsZone_script.cpp
  HangingLamp.cpp
  InfoPortion.cpp
  InventoryBox.cpp
  InventoryBox_script.cpp
  InventoryOwner.cpp
  InventoryOwner_script.cpp
  LevelDebugScript.cpp
  Level_network.cpp
  Level_network_spawn.cpp
  MosquitoBald_script.cpp
  PDA.cpp
  PhraseDialog_script.cpp
  PhysicObject.cpp
  PhysicObject_script.cpp
  RGD5.cpp
  Scope.cpp
  StalkerOutfit.cpp
  UIGameCustom.cpp
  UIGameCustom_script.cpp
  UIGameSP.cpp
  WeaponAmmo_script.cpp
  WeaponBinocularsVision.cpp
  WeaponFN2000.cpp
  WeaponFORT.cpp
  WeaponHPSA.cpp
  WeaponHPSA_script.cpp
  WeaponKnife_script.cpp
  WeaponLR300.cpp
  WeaponPM.cpp
  WeaponRG6_script.cpp
  WeaponRPG7_script.cpp
  WeaponSSRS_script.cpp
  WeaponStatMgun.cpp
  WeaponStatMgunFire.cpp
  actor_script.cpp
  agent_manager_properties.cpp
  ai_crow_script.cpp
  alife_human_brain_script.cpp
  alife_monster_brain_script.cpp
  alife_monster_detail_path_manager_script.cpp
  alife_monster_movement_manager_script.cpp
  alife_monster_patrol_path_manager_script.cpp
  alife_simulator.cpp
  alife_simulator_base.cpp
  alife_smart_terrain_task_script.cpp
  alife_surge_manager.cpp
  antirad_script.cpp
  client_spawn_manager_script.cpp
  cover_point_script.cpp
  danger_manager.cpp
  detail_path_manager.cpp
  eatable_item_script.cpp
  ef_storage_script.cpp
  entity_alive.cpp
  game_base.cpp
  game_sv_item_respawner.cpp
  game_sv_single.cpp
  helicopter_script.cpp
  hit_memory_manager.cpp
  holder_custom_script.cpp
  inventory_item_object.cpp
  inventory_item_script.cpp
  inventory_item_upgrade.cpp
  inventory_upgrade_base.cpp
  inventory_upgrade_group.cpp
  inventory_upgrade_manager.cpp
  inventory_upgrade_root.cpp
  map_manager.cpp
  medkit_script.cpp
  memory_space_script.cpp
  object_handler.cpp
  particle_params_script.cpp
  patrol_path_params_script.cpp
  physics_shell_scripted.cpp
  physics_world_scripted.cpp
  player_hud_script.cpp
  property_storage_script.cpp
  raypick.cpp
  relation_registry_actions.cpp
  saved_game_wrapper_script.cpp
  script_action_condition_script.cpp
  script_action_planner_action_wrapper.cpp
  script_action_wrapper.cpp
  script_animation_action_script.cpp
  script_attachment_script.cpp
  script_binder.cpp
  script_binder_object_script.cpp
  script_binder_object_wrapper.cpp
  script_effector_wrapper.cpp
  script_entity_action_script.cpp
  script_game_object2.cpp
  script_game_object3.cpp
  script_game_object4.cpp
  script_game_object_script.cpp
  script_game_object_script_trader.cpp
  script_game_object_smart_covers.cpp
  script_game_object_trader.cpp
  script_game_object_use2.cpp
  script_hit_script.cpp
  script_lanim.cpp
  script_light_script.cpp
  script_monster_action_script.cpp
  script_monster_hit_info_script.cpp
  script_movement_action_script.cpp
  script_object_action_script.cpp
  script_particle_action_script.cpp
  script_particles_script.cpp
  script_sound_action_script.cpp
  script_sound_info_script.cpp
  script_sound_script.cpp
  script_watch_action_script.cpp
  script_world_property_script.cpp
  script_zone_script.cpp
  smart_cover_animation_selector.cpp
  smart_cover_default_behaviour_planner.cpp
  smart_cover_evaluators.cpp
  smart_cover_object_script.cpp
  smart_cover_planner_target_provider.cpp
  smart_cover_transition_animation.cpp
  sound_memory_manager.cpp
  space_restrictor_script.cpp
  stalker_alife_actions.cpp
  stalker_animation_script.cpp
  stalker_anomaly_actions.cpp
  stalker_combat_action_base.cpp
  stalker_danger_planner.cpp
  stalker_danger_property_evaluators.cpp
  stalker_movement_manager_smart_cover_fov_range.cpp
  stalker_property_evaluators.cpp
  stalker_search_actions.cpp
  stalker_sound_data_visitor.cpp
  torch_script.cpp
  trade.cpp
  trade2.cpp
  trade_parameters.cpp
  visual_memory_manager.cpp
  weaponshotgun_script.cpp
  xrClientsPool.cpp
  xrServer_CL_connect.cpp
  xrServer_CL_disconnect.cpp
  xrServer_Disconnect.cpp
  xrServer_balance.cpp
  xrServer_perform_GameExport.cpp
  xrServer_perform_RPgen.cpp
  xrServer_perform_migration.cpp
  xrServer_perform_sls_default.cpp
  xrServer_perform_sls_load.cpp
  xrServer_perform_sls_save.cpp
  xrServer_perform_transfer.cpp
  xrServer_process_event.cpp
  xrServer_process_event_activate.cpp
  xrServer_process_event_destroy.cpp
  xrServer_process_event_ownership.cpp
  xrServer_process_event_reject.cpp
  xrServer_process_update.cpp
  xrServer_secure_messaging.cpp
  xrServer_sls_clear.cpp
)

# xrGame/ root sweep, second batch: 201 more files that compile clean once
# the ~686-file-class systemic bug was found to recur in a form nobody had
# hit yet - 306 xrGame files (mostly untouched-until-now root files) still
# spelled their OWN #include "stdafx.h" lowercase, which on this case-
# sensitive filesystem silently resolved to the WRONG file entirely
# (xrCore/stdafx.h, also lowercase and also on xrGame's own include path)
# instead of erroring - producing bizarre-looking cascading parse errors
# far downstream (e.g. "DLL_Pure does not name a type", because
# ENGINE_API never got #define'd) rather than a clean "file not found".
# Fixed project-wide via sed across all 306 real occurrences (not just
# these 201 - many more already-wired or still-blocked files had the same
# latent bug, now corrected as a side effect with zero behavior change).
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch2"
  FILES
  AI_PhraseDialogManager.cpp
  ActivatingCharCollisionDelay.cpp
  ActorAnimation.cpp
  ActorBackpack.cpp
  ActorCameras.cpp
  ActorEffector.cpp
  ActorHelmet.cpp
  ActorNightVision.cpp
  Actor_Events.cpp
  Actor_Feel.cpp
  Actor_Movement.cpp
  Actor_Weapon.cpp
  AdvancedDetector.cpp
  AmebaZone.cpp
  BastArtifact.cpp
  BlackDrops.cpp
  BlackGraviArtifact.cpp
  Bolt.cpp
  BoneProtections.cpp
  BottleItem.cpp
  BreakableObject.cpp
  CharacterPhysicsSupport.cpp
  CustomDetector.cpp
  CustomOutfit.cpp
  CustomRocket.cpp
  CustomZone.cpp
  DBG_Car.cpp
  DemoInfo_Loader.cpp
  DemoPLay_Control.cpp
  Dosimeter.cpp
  DummyArtifact.cpp
  EffectorBobbing.cpp
  EffectorShot.cpp
  EffectorShotX.cpp
  EffectorZoomInertion.cpp
  ElectricBall.cpp
  EliteDetector.cpp
  Entity.cpp
  EntityCondition.cpp
  ExoOutfit.cpp
  Explosive.cpp
  ExplosiveItem.cpp
  ExplosiveRocket.cpp
  FadedBall.cpp
  Flashlight.cpp
  FoodItem.cpp
  GalantineArtifact.cpp
  GraviArtifact.cpp
  GraviZone.cpp
  HUDRecon.cpp
  HUDTarget.cpp
  Hit.cpp
  HitMarker.cpp
  HolderEntityObject.cpp
  IKLimbsController.cpp
  InfoDocument.cpp
  Level_SLS_Default.cpp
  Level_SLS_Load.cpp
  Level_SLS_Save.cpp
  Level_load.cpp
  Level_network_compressed_updates.cpp
  Level_network_digest_computer.cpp
  Level_network_map_sync.cpp
  Level_network_messages.cpp
  Level_network_start_client.cpp
  Level_secure_messaging.cpp
  MercuryBall.cpp
  MilitaryOutfit.cpp
  Missile.cpp
  MosquitoBald.cpp
  Needles.cpp
  NoGravityZone.cpp
  PHCollisionDamageReceiver.cpp
  PHDestroyable.cpp
  PHDestroyableNotificate.cpp
  PHMovementControl.cpp
  Phrase.cpp
  PhraseDialogManager.cpp
  PhysicsSkeletonObject.cpp
  RadioactiveZone.cpp
  RocketLauncher.cpp
  RustyHairArtifact.cpp
  ScientificOutfit.cpp
  ShootingObject.cpp
  SimpleDetector.cpp
  ThornArtifact.cpp
  Torch.cpp
  TorridZone.cpp
  UIAchivementsIndicator.cpp
  UIDialogHolder.cpp
  UIZoneMap.cpp
  ZoneCampfire.cpp
  ZoneVisual.cpp
  ZudaArtifact.cpp
  actor_input_handler.cpp
  actor_memory.cpp
  actor_statistic_mgr.cpp
  agent_corpse_manager.cpp
  agent_explosive_manager.cpp
  agent_location_manager.cpp
  agent_manager.cpp
  agent_manager_actions.cpp
  agent_member_manager.cpp
  agent_memory_manager.cpp
  ai_space.cpp
  ai_stalker_alife.cpp
  antirad.cpp
  artefact_activation.cpp
  attachable_item.cpp
  attachment_owner.cpp
  autosave_manager.cpp
  cameralook.cpp
  character_hit_animations.cpp
  character_shell_control.cpp
  cover_evaluators.cpp
  cover_manager.cpp
  danger_explosive.cpp
  danger_object.cpp
  debug_text_tree.cpp
  dynamic_obstacles_avoider.cpp
  eatable_item.cpp
  eatable_item_object.cpp
  ef_primary.cpp
  encyclopedia_article.cpp
  file_transfer.cpp
  filetransfer_node.cpp
  fire_disp_controller.cpp
  first_bullet_controller.cpp
  flare.cpp
  game_cl_single.cpp
  game_news.cpp
  group_hierarchy_holder.cpp
  holder_custom.cpp
  hud_item_object.cpp
  inventory_quickswitch.cpp
  level_changer.cpp
  level_sounds.cpp
  map_spot.cpp
  material_manager.cpp
  medkit.cpp
  moving_object.cpp
  moving_objects.cpp
  moving_objects_dynamic.cpp
  moving_objects_dynamic_collision.cpp
  moving_objects_static.cpp
  object_property_evaluators.cpp
  patrol_point.cpp
  physic_item.cpp
  player_hud.cpp
  pp_effector_custom.cpp
  purchase_list.cpp
  rat_states.cpp
  relation_registry.cpp
  restricted_object.cpp
  saved_game_wrapper.cpp
  script_binder_object.cpp
  script_effector.cpp
  script_entity_action.cpp
  script_monster_action.cpp
  script_movement_action.cpp
  script_object_action.cpp
  script_particle_action.cpp
  script_sound.cpp
  script_sound_action.cpp
  script_wallmarks_manager.cpp
  script_wallmarks_script.cpp
  script_watch_action.cpp
  searchlight.cpp
  searchlight_script.cpp
  server_entity_wrapper.cpp
  sight_action.cpp
  sight_manager_target.cpp
  sound_player.cpp
  space_restriction_holder.cpp
  space_restriction_manager.cpp
  stalker_alife_task_actions.cpp
  stalker_animation_callbacks.cpp
  stalker_animation_data_storage.cpp
  stalker_animation_global.cpp
  stalker_animation_head.cpp
  stalker_animation_legs.cpp
  stalker_animation_manager_debug.cpp
  stalker_animation_torso.cpp
  stalker_anomaly_planner.cpp
  stalker_base_action.cpp
  stalker_death_actions.cpp
  stalker_movement_manager_base.cpp
  stalker_movement_params.cpp
  stalker_sound_data.cpp
  static_obstacles_avoider.cpp
  step_manager.cpp
  string_table.cpp
  trajectories.cpp
  vision_client.cpp
  wallmark_manager.cpp
  xrServerMapSync.cpp
  xrServer_info.cpp
  xrServer_svclient_validation.cpp
  xrServer_updates_compressor.cpp
  xr_time.cpp
  zone_effector.cpp
)

# xrGame/ root sweep, third batch: 21 more files fixed via real per-file
# investigation (not another blanket systemic fix) - a chain of individual
# bugs, all familiar shapes from earlier in this project but newly found
# in these specific files: self-shadowing template parameters
# (graph_edge.h's CEdgeBase<_edge_weight_type,_vertex_type> re-typedef'd
# its own parameters), missing `typename`/`this->` for dependent-base
# members and types across object_actions.h/property_evaluator_const.h/
# property_evaluator_member.h/object_manager_inline.h/
# level_path_manager_inline.h/game_path_manager_inline.h/
# path_manager_game_inline.h/path_manager_game_vertex_inline.h/
# game_location_selector_inline.h, illegal `typename` before a bare
# CPathManager<...> type-id in path_manager_game.h/
# path_manager_game_vertex.h/path_manager_game_level.h (same class as
# path_manager_level.h's earlier fix), and the psAI_Flags/MASTER_GOLD
# dead-declaration fix (this time in the shared ai/ai_monsters_anims.h
# template, fixing it once for everything that includes that header
# rather than per-.cpp-file). Bonus: HelicopterMovementManager.cpp and
# the 4 alife_monster/alife_spawn_registry* files were leftover
# exclusions from the ORIGINAL graph-cycle sweep (notes section 30c,
# months earlier in this project) - now fixed as a side effect of this
# pass's dependent-base/typename fixes, not separately targeted.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch3"
  FILES
  Artefact.cpp
  HelicopterMovementManager.cpp
  alife_monster_detail_path_manager.cpp
  alife_monster_patrol_path_manager.cpp
  alife_spawn_registry.cpp
  alife_spawn_registry_spawn.cpp
  item_manager.cpp
  map_location.cpp
  memory_manager.cpp
  movement_manager.cpp
  movement_manager_game.cpp
  movement_manager_level.cpp
  movement_manager_patrol.cpp
  object_actions.cpp
  patrol_path.cpp
  patrol_path_params.cpp
  patrol_path_storage.cpp
  smart_cover_planner_actions.cpp
  stalker_movement_manager_obstacles.cpp
  stalker_movement_manager_obstacles_path.cpp
  stalker_movement_manager_smart_cover_loopholes.cpp
)

# xrGame/ root sweep, fourth batch: 30 more files fixed via real
# per-file investigation, chasing each cascade's actual root cause
# rather than a blanket fix. Notable bugs found this pass:
# property_evaluator_const_inline.h/property_evaluator_member_inline.h
# missing `this->` for m_evaluator_name/m_storage (dependent-base
# members); script_callback_ex.h's `lua_state()` calls - doesn't exist
# in this fork's luabind::object/functor<T>, `interpreter()` is the real
# accessor; path_manager_level_flooder.h/path_manager_level_flooder_inline.h/
# path_manager_game_level_inline.h - the by-now-familiar illegal-`typename`-
# before-a-bare-CPathManager-type-id bug plus missing `this->` for
# dependent-base members, recurring in two more path-manager family
# members; space_restriction_bridge_inline.h/aimers_base_inline.h/
# alife_simulator_base_inline.h/static_cast_checked.hpp - missing direct
# #includes for types only forward-declared transitively (CLevelGraph,
# CBoneInstance, IKinematicsAnimated, CALifeRegistryContainer,
# object_type_traits); xrServerEntities/object_cloner.h - two illegal
# in-class explicit specializations of a member function template
# (CHelper::clone<true>, CHelper4::clone<true>), same `if constexpr`
# fix as problem_solver.h earlier in this port, PLUS two call sites
# missing the `template` disambiguator before a dependent template
# member (CHelper<T>::clone<...>, CHelper4<T>::clone<...>) - a genuine,
# previously-unexercised pre-existing bug in a shared header, not
# something this port introduced; xrCore/_sphere.h - `_sphere<T>::
# intersect_full()` used `Fsphere::` (typedef _sphere<float> Fsphere)
# to qualify its own class's enum members instead of using them
# unqualified, a genuine pre-existing bug in a foundational, already-
# compiling core header that only ever "worked" for T=float and was
# never exercised for any other instantiation until this pass - fixed
# by removing the (self-referential, and for T!=float simply wrong)
# qualification entirely, verified via a full clean rebuild of every
# already-compiling module with zero regressions.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch4"
  FILES
  Level_bullet_manager_firetrace.cpp
  actor_communication.cpp
  agent_manager_planner.cpp
  aimers_weapon.cpp
  alife_update_manager.cpp
  client_spawn_manager.cpp
  enemy_manager.cpp
  inventory_upgrade.cpp
  inventory_upgrade_property.cpp
  patrol_path_manager.cpp
  smart_cover_animation_planner.cpp
  smart_cover_planner_target_selector.cpp
  space_restriction.cpp
  space_restriction_bridge.cpp
  space_restriction_composition.cpp
  space_restriction_shape.cpp
  stalker_alife_planner.cpp
  stalker_animation_data.cpp
  stalker_animation_manager.cpp
  stalker_animation_manager_update.cpp
  stalker_animation_pair.cpp
  stalker_animation_state.cpp
  stalker_danger_by_sound_planner.cpp
  stalker_danger_grenade_planner.cpp
  stalker_danger_in_direction_planner.cpp
  stalker_danger_unknown_planner.cpp
  stalker_death_planner.cpp
  stalker_get_distance_planner.cpp
  stalker_low_cover_planner.cpp
  stalker_search_planner.cpp
)

target_link_libraries(xrGame PUBLIC xrCore xrCDB xrSound xrEngine xrPhysics xrNetServer)
