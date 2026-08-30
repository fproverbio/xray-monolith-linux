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
# script_engine_help.cpp: UPDATE (2026-08-30) - the "real rewrite, not a
# mechanical port" deferral below turned out to have a faithful modern
# replacement path after all, once the vendored fork's actual (not
# assumed) function-object machinery was traced end to end. Every
# obsolete API was ported to its real modern equivalent, with nothing
# dropped except one genuinely gone feature:
#   - method_rep/overload_rep/upvalue-3-magic-number-0x1337 detection ->
#     detail::is_luabind_function(L, index) (checks upvalue 2), then the
#     function_object* stored in upvalue 1, walked via ->next, formatted
#     per-overload via the virtual function_object::format_signature().
#     Same pattern reused for member functions, free functions, and (new)
#     constructor overloads - constructors turned out to be introspectable
#     the exact same way, since constructor_registration::register_()
#     (class.hpp) registers them through the identical make_function()/
#     add_overload() path used by every other method, just stored under
#     the "__init" key instead of a named method key.
#   - class_registry::iterate_classes() -> manual iteration over
#     get_classes() (a real luabind::map<type_id, class_rep*>), calling
#     print_class per entry.
#   - luabind::get_globals(L) -> luabind::globals(L) (per notes 25c/27d).
#   - luabind::internal_string -> luabind::string (still a real,
#     object_cast-able type, just renamed).
#   - object::pushvalue()/object::set() -> object::push(L) / the
#     object(from_stack(L,-1)) idiom used throughout the fork's own code.
#   - o.lua_state() -> o.interpreter().
#   - _gcvt_s/_CVTBUFSIZE (MSVC-only CRT) -> xr_sprintf with "%.16g",
#     matching the project's established float-to-string convention.
# The one dropped feature: crep->properties() (a distinct property-name
# container) no longer exists - class_rep stopped tracking properties
# separately. Recovered without loss of information by detecting property
# closures directly wherever they appear in the class' method table via
# their lua_CFunction entry point, `&detail::property_tag` - the same
# marker/idiom already used elsewhere in this exact fork
# (src/class_info.cpp, src/object_rep.cpp) to distinguish properties from
# regular methods, so properties are still listed as their own category.
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
  ui/UIMotionIcon.cpp
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
# is fixed too (path_manager_game_level_inline.h's this-> fixes). UPDATE
# (2026-08-29): the last 8 - WeaponBinocularsVision.cpp,
# WeaponMagazined.cpp, WeaponMagazinedWGrenade.cpp, WeaponStatMgun.cpp,
# WeaponStatMgunFire.cpp, WeaponStatMgunMisc.cpp, alife_surge_manager.cpp,
# alife_trader_abstract.cpp - are ALSO fixed now: verified each already
# compiled clean and was already an active (uncommented) entry below
# (added incidentally across Game4_RootBatch1/5/6, whichever pass's
# shared-header/dependent-base fixes happened to unblock it), this
# comment block just never got cleaned up to say so. Confirmed via a real
# full clean rebuild (1900/1900, 0 errors), not just standalone
# per-file -c checks. The graph-cycle sweep leftover list from §30c is
# now fully empty - 0 remaining. See the notes file's per-batch section
# for the exact original root-cause diagnosis this comment used to carry
# in full.

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

# xrGame/ root sweep, fifth batch: 24 more files fixed via real per-file
# investigation. Two genuine, previously-dormant pre-existing bugs found
# this pass in already-compiling foundational code (same class as
# Game4_RootBatch4's xrCore/_sphere.h find): xrCore/intrusive_ptr.h's
# ref_count_storage<>::intrusive_ref_sub() was missing a semicolon - a hard
# syntax error that never surfaced because nothing had instantiated that
# exact member function before (GCC's -fpermissive lets template bodies
# slide until first real instantiation, same root cause as _sphere.h);
# mixed_delegate.h's DEFINE_MIXED_DELEGATE_SCRIPT macro passed an
# overloaded (template + non-template) member function pointer to
# luabind's .def() with no disambiguation, needing the by-now-established
# `(Ret (Class::*)(Args...))&Class::method` cast. Also fixed: two more
# `luabind::object::type()` API-drift call sites (Actor.cpp/HudItem.cpp/
# WeaponStatMgunMisc.cpp - `luabind::type()` is the real accessor in this
# fork); property_evaluator_const.h/property_evaluator_member.h had
# accidentally moved `_value_type`/`_condition_type` from public (matching
# the CPropertyEvaluator base) to protected when an earlier pass added the
# `typename inherited::` qualification - a real regression in an already-
# "fixed" file; `return_stl_iterator`/`out_value<N>`/`adopt<N>` are alias
# templates in this fork's luabind (not the global tag objects the classic
# API docs show) - bare `return_stl_iterator` needs `()` to construct an
# instance, and `out_value<N>()`/`adopt<N>()` need `using namespace
# luabind::policy;` in scope (game_graph_script.cpp, alife_simulator_
# script.cpp, script_game_object_script3.cpp); alife_simulator_script.cpp's
# `instance = std::move(instance).enum_(...)[...]` was a real bug -
# enum_maker::operator[] already mutates and returns `instance` by
# reference, so the assignment was self-assignment through class_<>'s
# private operator=, fixed by dropping the assignment entirely;
# console_commands.cpp's Concat2/Concat3 macros used `##` to paste string
# literals together (illegal - `##` pastes preprocessor tokens, not
# strings), when plain adjacency was all that was needed since adjacent
# string literals concatenate automatically; Level_input.cpp/MainMenu.cpp
# each had a manual Win32 mouse-cursor-confinement block (ClientToScreen+
# SetCursorPos / GetClientRect+MapWindowPoints+ClipCursor) already made
# redundant by this port's SDL_SetRelativeMouseMode/SDL_SetWindowGrab
# usage elsewhere - stubbed out rather than ported; MainMenu.cpp also had
# 5 dead GameSpy-account-management includes (account_manager.h/
# login_manager.h/profile_store.h/stats_submitter.h/atlas_submit_queue.h)
# whose only uses were already commented out - removed; ActorInput.cpp's
# `det->ToggleDetector(...)` doesn't exist on CCustomDetector, real method
# is the inherited CCustomDevice::ToggleDevice(); HUDManager.cpp's DEBUG-
# only SetFirstBulletCrosshairDisp() called two nonexistent members
# (GetHUDCrosshair()/SetFirstBulletDispertion()) - stubbed to match its
# sibling SetDispersion(), already an empty stub in this fork;
# xrgame_dll_detach.cpp referenced g_profiler/CProfiler (doesn't exist -
# xrCore/profiler.h is a macro-only shim, not a class) and
# release_smart_cast_stats() (declared extern, but its definition in
# xrServerEntities/smart_cast_stats.cpp is entirely commented out) - both
# dead legacy debug diagnostics, removed. Remaining fixes were the
# established this->/typename/case/psAI_Flags-extern/sprintf_s-shim
# patterns from earlier batches.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch5"
  FILES
  Actor.cpp
  ActorInput.cpp
  HUDManager.cpp
  HudItem.cpp
  Level_input.cpp
  MainMenu.cpp
  PhraseDialog.cpp
  UIDosimeter.cpp
  WeaponAK74.cpp
  WeaponStatMgunMisc.cpp
  alife_simulator_script.cpp
  artefact_script.cpp
  console_commands.cpp
  game_graph_script.cpp
  inventory_item.cpp
  key_binding_registrator_script.cpp
  player_hud_legs.cpp
  profile_data_types_script.cpp
  property_evaluator_script.cpp
  script_game_object_inventory_owner.cpp
  script_game_object_script3.cpp
  script_property_evaluator_wrapper.cpp
  smart_cover_storage.cpp
  xrgame_dll_detach.cpp
)

# xrGame root: 16 files OUT OF SCOPE by deliberate project decision, not a
# TODO - same reasoning as ui/'s 5 exclusions above (this port targets
# singleplayer only, no functional multiplayer). The first 16 fatal-error
# on a `#include` for a header confirmed (via `find -iname`, not just a
# guess) to not exist anywhere in this source tree - genuine MP/GameSpy
# subsystem gaps from the same removal (commit 1d4ec53e) that produced
# ui/'s exclusions, not case/path bugs:
#   DemoInfo.cpp xrServer.cpp                    - game_sv_mp.h / game_sv_mp.h
#   Level_Bullet_Manager.cpp Spectator.cpp
#   WeaponFire.cpp game_cl_base_weapon_usage_statistic.cpp
#   screenshot_manager.cpp screenshot_server.cpp  - all game_cl_mp.h
#   game_cl_base.cpp                              - game_sv_mp_vote_flags.h
#   game_sv_base.cpp Level_start.cpp              - xrGameSpyServer.h
#   player_account.cpp ui_export_script.cpp       - login_manager.h
#   Level_network_Demo.cpp                        - UIGameDM.h
#   xrServer_Connect.cpp                          - game_sv_deathmatch.h
# ScriptXMLInit.cpp (2026-08-30): un-excluded - ui\ServerList.h (CServerList,
# the MP server-browser UI) was its only dead touchpoint, isolated the same
# way object_factory_register.cpp's was (one method + one .def() registration
# removed, everything else here is real generic XML/UI-init scripting glue
# used throughout, e.g. XMLLuaCallback, needed by the already-wired-in
# xrXMLParser.cpp).
# A 17th, game_cl_base_weapon_usage_statistic_save.cpp, is different in
# kind: it compiles fine on its own merits but both its functions
# (SaveData()/SaveDataLtx()) are called from nowhere in this entire
# source tree, and SaveData() itself only ever produces meaningful output
# for 4 explicit MP game-mode IDs (eGameIDDeathmatch et al.), early-
# returning for singleplayer - genuinely dead MP-only code, not worth
# porting SYSTEMTIME/GetLocalTime for.
# (GameObject.cpp also #included game_cl_mp.h, for one dead
# `if (GameID() != eGameIDSingle)`-gated reward-tracking block - unlike
# the 17 above, GameObject.cpp is core SP infrastructure with only that
# one incidental MP touchpoint, so it was fixed for real (dead block
# removed) rather than excluded; see Game4_RootBatch5 above.)

# xrGame/ root sweep, sixth batch: 65 more files fixed via real per-file
# investigation. Two more dead-code discoveries (matching the project's
# standing practice of stubbing subsystems this fork never carried
# forward, rather than porting them): physics_game.cpp's and
# script_game_object.cpp's `__try`/`__except(EXCEPTION_EXECUTE_HANDLER)`
# blocks were Win32 SEH (no portable equivalent - not a thing on Linux),
# used only to fall through to an existing unprotected fallback path in
# each case, so both were simply dropped; Level.cpp's spawn-antifreeze
# background-thread signal (SetEvent/ResetEvent/CreateEvent/
# WaitForSingleObject) got a real portable implementation added to
# xrCore/posix_sync.h (eventfd-backed, extending the CreateMutex pattern
# already there) since it's actively-used real functionality, not dead
# code. Two more genuinely missing luabind features found and added to
# the vendored fork (matching functor.hpp's own precedent from an earlier
# batch): `luabind::detail::meta::operator+` for combining converter
# policies (`out_value<2>() + out_value<3>()` et al - this fork only had
# `operator|`) added to meta.hpp; hash_fixed_vertex_manager::
# to_u32(shared_str const&) needed moving from two different .cpp files
# (each had their own copy, placed too late in their own translation
# unit to be visible where vertex_manager_hash_fixed_inline.h needed it)
# into graph_engine.h itself, alongside its existing CWorldState overload
# - deduplicated down to one definition in the right place, matching
# upstream `luabind::iterator` (a free type alias in this fork, not
# `luabind::object::iterator` as several files assumed) recurring across
# 6 files in the smart_cover/script_engine_help cluster. The usual
# recurring bug classes also appeared again at volume: `draw_obb`/
# `draw_line`/`draw_ellipse`/`draw_aabb` missing their trailing `bHud`
# bool argument (a dozen+ call sites across a dozen files); `psAI_Flags`
# needing the local `extern Flags32` workaround (8 more files);
# `luabind::object::type()` API drift (→ `luabind::type()`, several more
# files); the `class_<T, BaseOrBases, HolderType, WrapperType>` wrap_base
# argument-slot mixup (5 more registrations across
# action_base_script.cpp/action_planner_script.cpp/
# action_planner_action_script.cpp/script_effector_script.cpp/
# base_client_classes_script.cpp - the last one alone had 5 in one file);
# `EBodyState`/`EMovementType`/`EObjectAction` bare opaque-enum-forward-
# declarations (ai_monster_space.h's real definitions now included
# directly in stalker_combat_actions.h/stalker_low_cover_actions.h/
# object_handler_planner.h/smart_cover_transition.hpp, unblocking 9 more
# files); `auto& x = container.find(...)` binding a non-const reference
# to find()'s by-value return (6 more sites, GameObject.cpp/Level.cpp/
# script_attachment_manager.cpp). A few standalone real bugs: Inventory.
# cpp's `pIItem->GetSlot()` (method never existed) should have been the
# already-in-scope `slot_id` parameter; PhysicsShellHolder.cpp's
# `static enum EEnableState` (storage class on a type, illegal); Level.
# cpp's `goto spawn` crossing an intervening variable's initialization
# (wrapped the intervening block in braces); script_attachment_manager.
# cpp's `const_cast<T*>(m_userdata) = ...` (a no-op pointee-type cast used
# where a `const_cast<T*&>` reference cast was needed to actually
# reassign the member through a const member function); smart_cover_
# description.cpp's `delete_data()` template needing `typename` before
# several dependent nested types. `debug_make_final_test.cpp` and
# `static_cast_checked_test.cpp` are both deliberate "won't compile"
# self-tests (own comments say so) - correctly left excluded, not forced
# to compile.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch6"
  FILES
  ActorCondition_script.cpp
  ClimableObject.cpp
  CustomDevice.cpp
  GameObject.cpp
  HudSound.cpp
  Inventory.cpp
  Level.cpp
  MPPlayersBag.cpp
  Mincer.cpp
  PHScriptCall.cpp
  PHSkeleton.cpp
  PhraseScript.cpp
  PhysicsShellHolder.cpp
  TeleWhirlwind.cpp
  UICursor.cpp
  WeaponMagazined.cpp
  WeaponMagazinedWGrenade.cpp
  action_base_script.cpp
  action_planner_action_script.cpp
  action_planner_script.cpp
  alife_trader_abstract.cpp
  base_client_classes_script.cpp
  dbg_draw_frustum.cpp
  doors_actor.cpp
  inventory_owner_info.cpp
  level_debug.cpp
  level_graph_debug.cpp
  mincer_script.cpp
  object_handler_planner.cpp
  object_handler_planner_missile.cpp
  object_handler_planner_weapon.cpp
  physics_game.cpp
  player_hud_tune.cpp
  script_action_planner_wrapper.cpp
  script_attachment_manager.cpp
  script_effector_script.cpp
  script_entity.cpp
  script_game_object.cpp
  script_game_object_use.cpp
  script_world_state_script.cpp
  script_zone.cpp
  searchlight_struct.cpp
  sight_manager.cpp
  smart_cover.cpp
  smart_cover_action.cpp
  smart_cover_description.cpp
  smart_cover_loophole.cpp
  smart_cover_object.cpp
  smart_cover_transition.cpp
  space_restrictor.cpp
  stalker_combat_actions.cpp
  stalker_combat_planner.cpp
  stalker_danger_by_sound_actions.cpp
  stalker_danger_grenade_actions.cpp
  stalker_danger_in_direction_actions.cpp
  stalker_danger_unknown_actions.cpp
  stalker_get_distance_actions.cpp
  stalker_kill_wounded_actions.cpp
  stalker_kill_wounded_planner.cpp
  stalker_low_cover_actions.cpp
  stalker_movement_manager_smart_cover.cpp
  stalker_planner.cpp
  team_base_zone.cpp
  xrGame.cpp
  xrServer_process_spawn.cpp
)

# DynamicHeightMap.cpp (CHM_Static) is OUT OF SCOPE - not MP-related this
# time, but dead code: confirmed via grep that CHM_Static/CDynamicHeightMap
# are referenced nowhere else in the entire source tree, and the file
# relies on a RAPID-library BBox-triangle-query API (XRC.BBoxMode/
# BBoxCollide/BBoxContact/GetBBoxContactCount) that plain doesn't exist on
# this fork's xrXRC (xrCDB/xrXRC.h) - a much simpler ray/box/frustum-query
# wrapper around CDB::COLLIDER, an entirely different collision-query
# design. Reimplementing RAPID-style BBox triangle enumeration on top of
# CDB::COLLIDER for a class nothing instantiates isn't worth doing.

# xrGame/ root sweep, seventh batch: 9 more files fixed. One notable find:
# CustomMonster.cpp held the only definition of psAI_Flags in the entire
# codebase (`Flags32 psAI_Flags = {...};`), but it was #ifndef MASTER_GOLD-
# guarded exactly like ai_debug.h's own declaration - meaning it was
# ALSO dead in this build, and every `extern Flags32 psAI_Flags;` local
# workaround added across this whole root sweep (many files, several
# batches) had no real definition to resolve against at final-link time.
# Un-guarded it (and its aiObstaclesAvoiding/aiUseSmartCovers #define
# dependencies, same fix). Also: GamePersistent.cpp's 9 `__super::` calls
# (MSVC-only, not standard C++) replaced with the real explicit base class
# name (IGame_Persistent, confirmed for each overridden method), and its
# two dead `profiler()`/psAI_Flags(aiStats) diagnostic blocks removed
# (profiler() doesn't exist anywhere in this port, same class as the
# g_profiler/CProfiler removals elsewhere); agent_enemy_manager.cpp's
# `unsigned __int32` (MSVC extended-integer-type spelling, not standard)
# fixed to u32, plus two more `typename`-before-dependent-nested-type
# gaps (memory_manager_inline.h had `typename` in the wrong position
# entirely - after the `::` instead of before the whole qualified-id);
# doors_door.h/smart_cover_loophole_planner_actions.h needed the by-now-
# familiar real-header-instead-of-bare-forward-declare fix for
# door_state/EWorldProperties; level_graph_debug2.cpp had two real
# variable-scope bugs (a for-loop index needed to survive past loop end,
# only declared inside the loop's own init-statement) alongside its own
# `m_ref_count` accessor-rename instance. The recurring bHud-argument and
# psAI_Flags-extern patterns accounted for the rest.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch7"
  FILES
  Actor_Network.cpp
  CustomMonster.cpp
  GamePersistent.cpp
  PHDebug.cpp
  UsableScriptObject.cpp
  agent_enemy_manager.cpp
  level_graph_debug2.cpp
  smart_cover_detail.cpp
  smart_cover_loophole_planner_actions.cpp
)

# RegistryFuncs.cpp and profile_data_types.cpp are OUT OF SCOPE, both dead
# code like DynamicHeightMap.cpp above: RegistryFuncs.cpp's Read/
# WriteRegistry* functions (Win32 registry API - HKEY/RegOpenKeyEx/etc,
# no Linux equivalent) are called from nowhere in the entire tree;
# profile_data_types.cpp is entirely `namespace gamespy_profile` (GameSpy
# online-achievements/leaderboard stat names), the same MP/GameSpy
# subsystem family as player_account.cpp/ui_export_script.cpp above.

# xrGame/ root sweep, eighth batch: 4 more files fixed. level_script.cpp/
# script_imgui_script.cpp both needed only the missing `using namespace
# luabind::policy;` for out_value<N>()/pure_out_value<N>(). level_script.cpp also had a genuine `R_ASSERT`
# bug (passed 2 comma-separated conditions where the macro takes 1 -
# combined with `&&`, matching the obvious original intent) and a dead
# GetSystemMetrics(SM_SWAPBUTTON) left-handed-mouse check (X11 already
# swaps physical button events for a left-handed-configured mouse, same
# reasoning already established in Xr_input.cpp for the same API).
# script_game_object_script2.cpp needed two fixes of its own: SAFE_WRAP's
# non-type-template-parameter mechanism requires the `.def()` cast's
# return-type spelling to exactly match the real declaration including a
# meaningless-but-present top-level `const` (idle_min_time() et al. are
# declared `float const ... () const` in script_game_object.h); and
# `adopt<result>()` tried to use a meta::index<0> marker object where
# adopt<N> needs a real unsigned-int template argument - `adopt<0>()`.
# doors_door.cpp was unblocked as a side effect of the doors_door.h fix
# two batches ago.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch8"
  FILES
  doors_door.cpp
  level_script.cpp
  script_game_object_script2.cpp
  script_imgui_script.cpp
)

# death_anims.cpp - one of the 5 files that shared an identical, previously
# not-root-caused "aggregate ENGINE_API CBoneInstance has incomplete type"
# cascade out of xrEngine/bone.h (§41). Root cause found: this file's own
# `#include "stdafx.h"` is BOTH lowercase AND uses a literal TAB character
# between `#include` and the filename (`#include\t"stdafx.h"`) - the exact
# same lowercase-stdafx.h bug already fixed project-wide in an earlier pass
# (306 files, see the Game4_RootBatch2 comment above), just missed by that
# pass's sed pattern because it assumed a single space, not a tab, between
# `#include` and the opening quote. Same silent-wrong-file-found failure
# mode as before: `xrCore/stdafx.h` (lowercase, real, also on xrGame's
# include path) satisfies the lowercase include instead of erroring, so
# `xrEngine/stdafx.h` (reached only via the real `xrGame/StdAfx.h`) never
# runs and ENGINE_API never gets #define'd - by the time bone.h is reached,
# `class ENGINE_API CBoneInstance;` parses as declaring a variable named
# CBoneInstance of an incomplete type literally named ENGINE_API, producing
# the exact cascade seen. bone.h/KinematicsAnimated.h/animation_motion.h
# were correctly ruled out as not at fault in §41 - confirmed here for
# real. Fixed by capitalizing to `StdAfx.h` (tab preserved, only the
# filename case changed). The other 4 files sharing this exact cascade
# (death_anims_predicates.cpp, ik_calculate_data.cpp, ik_dbg_matrix.cpp,
# ik_foot_collider.cpp) have the identical tab-separated lowercase
# #include and are expected to need the identical fix - not yet
# individually verified, left for their own dedicated passes.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch9"
  FILES
  death_anims.cpp
)

# death_anims_predicates.cpp - identical tab-separated lowercase
# #include "stdafx.h" bug as death_anims.cpp above, confirmed via the
# same "aggregate ENGINE_API ... has incomplete type" cascade (this time
# through pure_relcase.h/Feel_Touch.h/Feel_Sound.h/IInputReceiver.h/
# bone.h in sequence - same root cause, different first-touched headers
# since this file's own include order differs). Fixed identically.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch10"
  FILES
  death_anims_predicates.cpp
)

# ik_calculate_data.cpp - same tab-separated lowercase stdafx.h bug as
# death_anims.cpp/death_anims_predicates.cpp above.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch11"
  FILES
  ik_calculate_data.cpp
)

# ik_dbg_matrix.cpp - same tab-separated lowercase stdafx.h bug as the
# other bone.h-cascade files above.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch12"
  FILES
  ik_dbg_matrix.cpp
)

# ik_foot_collider.cpp - same tab-separated lowercase stdafx.h bug as the
# other bone.h-cascade files above. This is the last of the 5 files that
# shared the §41 "aggregate ENGINE_API CBoneInstance has incomplete type"
# cascade - all 5 turned out to be the identical root cause (a tab
# character between #include and "stdafx.h" that the original project-
# wide lowercase-stdafx.h sed fix's space-only pattern missed), not a
# genuine bone.h/KinematicsAnimated.h/animation_motion.h defect.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch13"
  FILES
  ik_foot_collider.cpp
)

# physics_joint_scripted.cpp - the "out_value<2>()+out_value<3>() gap on
# a member function" from §41 was a MISATTRIBUTION, not a real luabind
# limitation. A standalone isolated repro (a throwaway class registered
# with the exact same `void GetLimits(float&, float&, int)`-shaped
# member function and `out_value<2>() + out_value<3>()`) compiled with
# ZERO errors, proving the combinator works fine on member functions.
# The real bug: `get_max_force_and_velocity`'s own `.def()` call, right
# above `get_limits` in this file, registers
# `GetMaxForceAndVelocity(float& force, float& velocity, int axis_num)`
# - an IDENTICAL (float&, float&, int) shape - with NO out_value policy
# at all, so it hits the exact same "default_converter<float&,void> has
# no match() for by_reference<float>" error text as get_limits would.
# Because `register_()` is a virtual override (instantiated whenever the
# class template is, active or not) and appears earlier in the same
# fluent `.def().def()...` chain, its errors were reported first and
# looked identical enough to get_limits' own (correctly-written)
# out_value<2>()+out_value<3>() call to be misread as the same bug in
# the previous session. Confirmed by isolating: temporarily replacing
# get_limits' body with a harmless stand-in left the exact same 3 errors
# in place, now unambiguously pointing at get_max_force_and_velocity.
# Fixed by adding the same out_value<2>() + out_value<3>() policy there.
# GetAxisDirDynamic(int, Fvector&)/GetAnchorDynamic(Fvector&) need no
# such policy - class-type references convert natively, only primitive
# (float&, int&, etc) out-params need it.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch14"
  FILES
  physics_joint_scripted.cpp
)

target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch15"
  FILES
  script_engine_help.cpp
)

# xrServerEntities: a whole ~60-file module (script engine core, ALife
# entity/trader/character classes, xrServer object replication) that was
# NEVER wired into this port's build at all - discovered only once a real
# executable link was first attempted (2026-08-30). The original
# xrGame.vcxproj confirms these files were always meant to compile
# directly into xrGame itself (58 <ClCompile Include="..\xrServerEntities\...">
# entries, no separate project) - not a separate module boundary, so they're
# added here via relative path rather than as their own CMake target.
# smart_cast.cpp/smart_cast_stats.cpp were genuinely absent from the real
# vcxproj too (only smart_cast.h is <ClInclude>-listed) - console_commands.cpp's
# show_smart_cast_stats()/clear_smart_cast_stats() calls (registered under
# #ifdef DEBUG, which this build's Debug config does define) were an
# unnoticed link-time landmine in the real upstream engine too, not just
# this port. UPDATE (2026-08-30, roadmap step 4): smart_cast_stats.cpp's
# entire body turned out to be commented out, including its own #ifdef
# SMART_CAST_STATS/#else guards - SMART_CAST_STATS/SMART_CAST_STATS_ALL are
# never defined anywhere in this tree, so the file's real stats-tracking
# code was already permanently dead upstream. Resurrected just the 3
# extern function signatures console_commands.cpp needs
# (show_smart_cast_stats/clear_smart_cast_stats/release_smart_cast_stats)
# with the file's own already-written #else-branch behavior (an
# "SMART_CAST_STATS is not defined, stats is disabled" log message) made
# real instead of a comment - not a new feature, just the documented
# fallback path finally compiled. See ServerEntities_Batch6 below.
# smart_cast.cpp remains excluded - real smart_cast<> dispatch itself,
# not a diagnostics/landmine file, out of scope to touch here.
#
# First batch, 25 of 58 files, wired via the same recon-scan-then-fix
# discipline as every other module's first batch:
#   - the same case-sensitive #include bug already fixed project-wide for
#     xrGame's own root files (§37) recurred here verbatim (level.h/Level.h,
#     Script_Game_Object.h/script_game_object.h, xrServer_Objects_ALife_
#     Smartcovers.h/xrServer_Objects_Alife_Smartcovers.h, etc.) - never
#     swept before since this module was never in the build to sweep.
#   - the same missing-`template<>`-before-an-explicit-member-specialization
#     bug (§37) recurred in all 6 script_f*/script_reader/script_net_packet
#     "primitive type script export" files (class_exporter<T>::script_register
#     defined out-of-line per-T without the required template<> keyword).
#   - `Flags16::or`/`Flags16::and`/`Flags32::or`/`Flags32::and` (real member
#     names) collide with the C++ alternative-token keywords `or`/`and` -
#     MSVC never enforces these as keywords by default, GCC always does.
#     The real methods are named `or_`/`and_` (xrCore/_flags.h) - someone
#     already anticipated this exact collision when writing the class, the
#     .cpp registering them just hadn't been updated to match.
#   - `&set<Flags16>` ambiguous against `std::set` once <set> becomes
#     visible transitively - qualified as `&::set<Flags16>` (the real
#     free function template is file-local, at global scope).
#   - `class_<T>::scope` is a data member (`detail::static_scope<self_t>`),
#     not a method - `.scope()[...]` needed to become `.scope[...]`.
#   - the out_value/pure_out_value-policy-missing-on-a-reference-out-param
#     bug (§44) recurred at scale: NET_Packet's and IReader's many typed
#     r_*(T&)-shaped accessors were registered with no policy at all.
#     NOTE: NET_Packet and IReader do NOT share the same overload set for
#     r_angle16/r_angle8/r_float_q16/r_float_q8 despite identical names -
#     NET_Packet's take a `float&` out-param, IReader's (inherited from
#     IReaderBase<T>, xrCore/FS.h) return by value with no out-param at
#     all. Copying one file's fix onto the other by name-pattern alone is
#     wrong; always check the real declaration per class.
#   - `_matrix<T>::hud_to_world()`/`world_to_hud()` and their `_vector3<T>`
#     equivalents (xrCore/_matrix.h, _vector3d.h) call a `Device.*` method -
#     `Device` is an xrEngine global that xrCore, architecturally, has no
#     business referencing. GCC's `-Wtemplate-body` poisons the whole
#     template the instant its body is first PARSED (not instantiated),
#     regardless of what the including TU does afterward - not fixable by
#     reordering includes. Dropped the 6 Lua registrations that would have
#     instantiated these (2 in script_fmatrix_script.cpp, 4 in
#     script_fvector_script.cpp) rather than chasing a foundational xrCore
#     fix for obscure HUD/camera-transform convenience wrappers nothing
#     else has ever needed.
#   - `IReader::r_float_q16`/`r_float_q8` (script_reader_script.cpp) do NOT
#     need any out_value-family policy at all (return-by-value, no
#     reference param) - adding one produces a confusing several-layers-
#     deep template error that looks unrelated to the real mistake.
# Remaining 33 files (higher error counts: character_info.cpp 65,
# lua_studio.cpp 82, specific_character.cpp reaches script_game_object.h's
# 156-includer hub once its own case-#include bug is fixed, several
# xrServer_Objects_ALife_* files with 3-13 errors each) not yet
# investigated - good next batch.
target_sources_grouped(
  TARGET xrGame
  NAME "ServerEntities_Batch1"
  FILES
  ../xrServerEntities/PHNetState.cpp
  ../xrServerEntities/alife_human_brain.cpp
  ../xrServerEntities/alife_monster_brain.cpp
  ../xrServerEntities/alife_space.cpp
  ../xrServerEntities/pch_script.cpp
  ../xrServerEntities/script_callStack.cpp
  ../xrServerEntities/script_debugger_threads.cpp
  ../xrServerEntities/script_fcolor_script.cpp
  ../xrServerEntities/script_flags_script.cpp
  ../xrServerEntities/script_fmatrix_script.cpp
  ../xrServerEntities/script_fvector_script.cpp
  ../xrServerEntities/script_ini_file.cpp
  ../xrServerEntities/script_lua_helper.cpp
  ../xrServerEntities/script_net_packet_script.cpp
  ../xrServerEntities/script_process.cpp
  ../xrServerEntities/script_reader_script.cpp
  ../xrServerEntities/script_rtoken_list_script.cpp
  ../xrServerEntities/script_stack_tracker.cpp
  ../xrServerEntities/script_thread.cpp
  ../xrServerEntities/script_token_list.cpp
  ../xrServerEntities/script_token_list_script.cpp
  ../xrServerEntities/xrServer_Objects_ALife.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Items.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Monsters.cpp
  ../xrServerEntities/xrServer_Objects_Alife_Smartcovers.cpp
)

# xrServerEntities batch 2: 14 files, unlocked almost entirely by a single
# root-cause fix rather than per-file triage. `xrServer_script_macroses.h`'s
# `luabind_class_*N` macro family (used by every server-entity class's Lua
# binding registration) expands to `DEFINE_LUABIND_CLASS_WRAPPER_N`, which
# passed the polymorphism-dispatch wrapper type (CWrapperAbstractItem<T> et
# al., a luabind::wrap_base subclass) as class_<>'s 3rd template argument
# (HolderType) instead of its 4th (WrapperType) - see
# script_export_macroses.h's own comment above DEFINE_LUABIND_CLASS_WRAPPER_0
# for the full mechanism. This macro system predates the 4-parameter
# class_<T, BaseOrBases, HolderType, WrapperType> shape in this fork's
# bundled luabind; every registration compiled by luck only because
# WrapperType silently defaulted to null_type, so add_wrapper_cast() and
# constructor_registration() never actually needed the real wrapper type's
# definition - until a class with both a real constructor registration and
# this positional bug got exercised (xrServer_Objects_ALife_Items_script3.cpp
# et al.), which failed deep in luabind/detail/constructor.hpp trying to
# construct e.g. `CWrapperAbstractItem<CSE_ALifeItemWeaponMagazinedWGL>`
# directly from a raw T* (it only has `explicit CWrapperXxx(LPCSTR)`).
# Moving `wrapper` to the 4th slot (and passing `luabind::null_type`
# explicitly for the 3rd) fixed the root cause for all 5 macro arities in
# one place.
# Fixing that exposed a second, previously-dormant bug in the same header:
# CWrapperAbstractItem<T> (and its siblings) is a real class template
# defined at global scope, and every INHERIT_ITEM/INHERIT_MONSTER/etc-
# generated method called the free function `call_member<ret_type>(this,
# ...)` completely unqualified. `call_member` is only ever found via ADL
# (deferred to instantiation/phase 2), but the compiler must already know
# at phase 1 whether a name denotes a template to parse the following `<`
# as a template-argument-list opener rather than less-than - and ordinary
# (non-ADL) lookup at the template's definition point (before any .cpp's
# own `using namespace luabind;`, which comes too late to help a header
# parsed earlier) finds nothing. GCC's -Wtemplate-body flags this
# ambiguity as a warning at first parse but then hard-poisons the template,
# turning any real instantiation into "instantiating erroneous template" -
# the same poisoning mechanism as the already-documented Device/_matrix.h
# finding above, different root cause. Fixed by qualifying every
# `call_member<...>` call in script_export_macroses.h as
# `luabind::call_member<...>` (a qualified-id is unambiguous regardless of
# lookup phase); `call<...>` calls were untouched since those are member
# calls on the non-dependent `wrap_base` base and were never affected.
# Two other small independent fixes landed alongside this batch:
#   - object_factory_script.cpp: `instance = std::move(instance).enum_(...)
#     [...]` tried to assign back into a luabind::class_<>, whose
#     operator= is private (same class of bug as this file's own
#     console_commands.cpp precedent above) - dropped the redundant
#     self-assignment, since enum_()'s operator[] already mutates and
#     returns the original object by reference.
#   - script_engine_export.h/.cpp: several case-#include bugs (particlesobject.h/
#     level.h -> ParticlesObject.h/Level.h, demoinfo.h -> DemoInfo.h, etc.,
#     xrServer_Objects_ALife_Smartcovers.h -> ...Alife...); an include-order
#     bug (script_sound_type.h's `enum ESoundTypes;` is a use of an enum
#     only ever fully declared in ai_sounds.h, which the header included
#     too late - moved ai_sounds.h first); and 10 dead
#     gamespy_*::script_register() calls plus their 3 dead includes
#     (login_manager.h/account_manager.h/profile_store.h - confirmed absent
#     anywhere in this source tree, same GameSpy-removal class as this
#     file's other documented exclusions) - all 10 types have zero
#     definitions anywhere in the tree, so the calls were simply dead code.
target_sources_grouped(
  TARGET xrGame
  NAME "ServerEntities_Batch2"
  FILES
  ../xrServerEntities/object_factory_script.cpp
  ../xrServerEntities/script_engine_export.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Items_script.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Items_script2.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Items_script3.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Monsters_script.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Monsters_script2.cpp
  ../xrServerEntities/xrServer_Objects_ALife_Monsters_script4.cpp
  ../xrServerEntities/xrServer_Objects_ALife_script.cpp
  ../xrServerEntities/xrServer_Objects_ALife_script2.cpp
  ../xrServerEntities/xrServer_Objects_ALife_script3.cpp
  ../xrServerEntities/xrServer_Objects_Alife_Smartcovers_script.cpp
  ../xrServerEntities/xrServer_Objects_script.cpp
  ../xrServerEntities/xrServer_Objects_script2.cpp
)

# xrServerEntities batch 3: 8 files, mostly narrow independent bugs.
#   - xrserver_objects_alife_monsters_script3.cpp: `return_stl_iterator`
#     used bare instead of instantiated (`return_stl_iterator()` - it's a
#     type alias, not a value), plus the usual missing
#     `using namespace luabind::policy;`.
#   - LevelGameDef.cpp/gametype_chooser.cpp: both `#include "stdafx.h"`
#     (lowercase) instead of the real `xrGame/StdAfx.h` (capitalized) -
#     quoted-include search is case-sensitive and, given this build's
#     include-path order, silently resolves to the much narrower
#     `xrCore/stdafx.h` instead, which doesn't define `ECORE_API` -
#     LevelGameDef.h and Include/xrRender/DrawUtils.h (via
#     xrServer_Objects_Abstract.h) both use it. Fixed by capitalizing the
#     include; most other xrServerEntities files with this same lowercase
#     include happen to compile fine regardless, since their headers never
#     need anything from the wider xrGame/StdAfx.h chain.
#   - script_engine.cpp: `LUABIND_TYPE_INFO` (the old luabind's opaque
#     type-info typedef) no longer exists - this fork's bundled luabind's
#     cast-failed callback is `void(*)(lua_State*, luabind::type_id
#     const&)`; updated `lua_cast_failed`'s signature and `info->name()` ->
#     `info.name()` (type_id is passed by reference, not pointer) to match.
#   - script_sound_type.h: same enum-forward-declared-without-a-real-
#     declaration-in-scope bug already fixed once in script_engine_export.h
#     for this same include chain, but here it's a bug in the shared header
#     itself, not a downstream includer - fixed at the source instead of
#     per-includer, by including ai_sounds.h (the real declaration)
#     directly. Unlocked script_sound_type_script.cpp, which still needed
#     its own separate missing-`template<>` fix.
#   - object_item_script.h/.cpp: `luabind::object::operator()` (this fork's
#     bundled luabind) isn't const-qualified, but `m_client_creator`/
#     `m_server_creator` are invoked from const methods to call into Lua -
#     marked both members `mutable` (calling a stored Lua factory doesn't
#     change its identity). Also `luabind::adopt<luabind::result>()` - both
#     the type (needs `luabind::policy::` qualification, or `using
#     namespace luabind::policy;`) and the `result` constant (removed;
#     0 is its direct positional equivalent - already established in
#     xrGame/script_game_object_script2.cpp) needed updating.
#   - script_ini_file_script.cpp: `luabind::internal_string` doesn't exist
#     in this fork's bundled luabind - replaced with `luabind::string`
#     (`std::basic_string` with luabind's allocator), which is exactly
#     what internal_string aliased to in the original API and is already
#     used the same way elsewhere (xrGame/script_engine_help.cpp); plus the
#     same `adopt<result>()` -> `adopt<0>()` fix as object_item_script.cpp.
#   - script_engine_script.cpp: a real, narrow pre-existing bug - ErrorLog()
#     called `ai().script_engine().debugger()` guarded only by `#ifdef
#     DEBUG`, when `debugger()` only exists at all under `#ifdef
#     USE_DEBUGGER` (not defined in this build) - every other call site in
#     the same function correctly guards on USE_DEBUGGER too. Fixed by
#     adding the missing guard. Separately, `.def(tostring(self))` on
#     profile_timer_script needs `operator<<(ostream&, T const&)` to exist,
#     which was present in the source but commented out (non-const
#     signature) - restored it with a const-correct signature matching
#     luabind's tostring_operator<T>(T const&).
target_sources_grouped(
  TARGET xrGame
  NAME "ServerEntities_Batch3"
  FILES
  ../xrServerEntities/LevelGameDef.cpp
  ../xrServerEntities/gametype_chooser.cpp
  ../xrServerEntities/object_item_script.cpp
  ../xrServerEntities/script_engine.cpp
  ../xrServerEntities/script_engine_script.cpp
  ../xrServerEntities/script_ini_file_script.cpp
  ../xrServerEntities/script_sound_type_script.cpp
  ../xrServerEntities/xrserver_objects_alife_monsters_script3.cpp
)

# xrServerEntities batch 4: 6 more files wired in, plus one deliberate
# exclusion.
#   - object_factory.cpp/xrServer_Factory.cpp/xrServer_Object_Base.cpp/
#     xrServer_Objects.cpp/xrServer_Objects_Abstract.cpp: all 5 hit the
#     same `#include "stdafx.h"` (lowercase) case bug as batch 3's
#     LevelGameDef.cpp/gametype_chooser.cpp - every reported error
#     (DLL_Pure/CLIENT_BASE_CLASS/ECORE_API/Device/smart_cast, all looking
#     unrelated at first glance) was a downstream cascade from resolving to
#     the narrower xrCore/stdafx.h instead of xrGame/StdAfx.h. Fixing the
#     one include resolved 4 of the 5 files outright; xrServer_Object_Base.cpp
#     additionally had `static enum EGameTypes {...}` - storage-class
#     specifiers don't apply to type definitions, only to objects and
#     functions - MSVC apparently tolerated this, GCC doesn't; dropped the
#     stray `static`.
#   - script_storage.cpp needed several independent fixes: the same
#     psAI_Flags/aiLua-dead-under-MASTER_GOLD workaround already
#     established for xrGame/CustomMonster.cpp et al. (see that file's own
#     comment for why); `luabind::set_allocator(fn)` doesn't exist in this
#     fork's bundled luabind - it's a plain assignable global,
#     `luabind::allocator = fn`, whose function-pointer type also grew a
#     leading `void* context` parameter that the existing allocator
#     callback didn't have; a dangling `g_pScriptEngine->print_output(...)`
#     (no such global anywhere in the tree) where every other call site in
#     this same file calls the identical method as the static
#     `CScriptStorage::print_output(...)` it is; a `__try`/`__except
#     (GetExceptionCode() == STATUS_STACK_OVERFLOW)` SEH block working
#     around a stack-overflowing `_alloca()` - genuinely unportable (no SEH,
#     no `_resetstkoflw()` on Linux) and dropped following the same
#     established precedent as xrGame/script_game_object.cpp's and
#     xrGame/physics_game.cpp's own already-documented SEH removals, kept
#     safe here by the pre-existing < 768KB threshold that already keeps
#     `_alloca()` well clear of any realistic stack size; `luabind::get_globals`
#     -> `luabind::globals` (renamed); and the same missing-`USE_DEBUGGER`-
#     guard bug as batch 3's script_engine_script.cpp, recurring twice more
#     in this file.
#   - script_debugger.cpp (CScriptDebugger's actual implementation, using
#     Win32 Mailslot IPC - mslotutils.h's CreateMailslot/WriteFile/
#     LPSECURITY_ATTRIBUTES/etc, and MSVC's _splitpath family) is
#     deliberately NOT wired in. Investigating why exposed a real, subtle
#     finding: script_storage.h defines `USE_DEBUGGER`+`USE_LUA_STUDIO`
#     together, always, under one `#ifndef MASTER_GOLD` - and MASTER_GOLD
#     itself ends up always defined regardless of DEBUG, because xrCore.h
#     checks `#ifndef DEBUG` to set MASTER_GOLD (line ~5) *before* it
#     derives `DEBUG` from the compiler's own `_DEBUG` (line ~90) later in
#     the same file - so by the time MASTER_GOLD's guard runs, DEBUG isn't
#     defined yet no matter the build config. Every call site that reaches
#     `CScriptEngine::debugger()`'s `CScriptDebugger*`-returning overload is
#     itself guarded by `#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)`
#     (e.g. xrGame/console_commands.cpp's CCC_ScriptDbg, already confirmed
#     compiled out via `-E` preprocessor inspection) or by
#     `#ifndef USE_LUA_STUDIO` alone (script_engine.cpp/script_engine_inline.h) -
#     both permanently false here, so CScriptDebugger is real, working code
#     that is simply never reachable in this build configuration. No
#     currently-compiling file calls any of its methods (script_callStack.cpp/
#     script_lua_helper.cpp only ever store a `CScriptDebugger*` they're
#     handed, never dereference it), so excluding script_debugger.cpp costs
#     nothing at link time.
target_sources_grouped(
  TARGET xrGame
  NAME "ServerEntities_Batch4"
  FILES
  ../xrServerEntities/object_factory.cpp
  ../xrServerEntities/script_storage.cpp
  ../xrServerEntities/xrServer_Factory.cpp
  ../xrServerEntities/xrServer_Object_Base.cpp
  ../xrServerEntities/xrServer_Objects.cpp
  ../xrServerEntities/xrServer_Objects_Abstract.cpp
)

# xrServerEntities batch 5 (final): the last 3 real files, plus one more
# deliberate exclusion. The xrServerEntities module (58 real files total,
# discovered never wired in at all - see the batch 1 comment above) is
# now fully accounted for: 55 wired in across 5 batches, 2 confirmed
# genuinely dead (smart_cast.cpp/smart_cast_stats.cpp, §batch1),
# 1 confirmed permanently unreachable in this build (lua_studio.cpp,
# below).
#   - character_info.cpp: the lowercase `#include "stdafx.h"` case bug
#     again (65 reported errors, all one cascade from the same wrong
#     xrCore/stdafx.h resolution as batch 3/4's files) - fixing the one
#     include resolved every one of them.
#   - specific_character.cpp: same lowercase `stdafx.h` cascade (855(!)
#     reported errors, by far the largest single-fix payoff this project
#     has seen - one character swap resolved 847 of them), plus 5 real
#     `luabind::object::type()`/`::object::iterator` API-drift bugs (same
#     class already fixed a dozen+ times across xrGame, e.g.
#     xrGame/smart_cover_loophole.cpp - `luabind::iterator` is a free type
#     alias constructible from an object, not a nested `object::iterator`
#     type, and `type()` is a free function, not a method).
#   - object_factory_register.cpp: after the same lowercase-`stdafx.h` fix
#     and 2 directory-case include fixes (`ai/monsters/Burer/burer.h` ->
#     `burer/burer.h`, `PseudoGigant` -> `pseudogigant`) plus one
#     backslash-separated Windows-style include
#     (`..\xrGame\HolderEntityObject.h` -> `../xrGame/HolderEntityObject.h`,
#     GCC never treats `\` as a path separator), real MP/SP triage was
#     needed: this file mixes real singleplayer object registrations with
#     several genuinely-dead MP-only game modes and one dead artefact
#     item, all confirmed absent anywhere in this source tree via grep -
#     `game_sv_deathmatch.h`/`game_sv_teamdeathmatch.h`/
#     `game_sv_ArtefactHunt.h`/`game_sv_capture_the_artefact.h` and their
#     `game_cl_*` client-side counterparts (Deathmatch/TeamDeathmatch/
#     ArtefactHunt/CaptureTheArtefact game-mode classes), `UIGameDM.h`/
#     `UIGameTDM.h`/`UIGameAHunt.h`/`UIGameCTA.h` (their UI counterparts -
#     UIGameDM.h/UIGameTDM.h were never even in the original include list,
#     already broken before this pass touched the file, just masked by
#     the earlier fatal include errors), `cta_game_artefact.h`
#     (CtaGameArtefact, an MP-only artefact item - its one registration
#     was ALREADY commented out elsewhere in this same file, confirming
#     the precedent), and `actor_mp_server.h`/`actor_mp_client.h`
#     (CActorMP/CSE_ActorMP, the MP-aware actor variant used even in the
#     nominally-singleplayer code path via
#     `CObjectItemClientServerSingleMp<CActor, CActorMP, ...>` since
#     NO_SINGLE is never defined in this build). Removed each dead
#     include and commented out or replaced its corresponding
#     registration - for the actor specifically, changed to register the
#     plain singleplayer `CActor` directly (`ADD(CActor,
#     CSE_ALifeCreatureActor, CLSID_OBJECT_ACTOR, "actor")`, the same call
#     the file's own `#else // NO_XR_GAME` branch already uses) instead of
#     wrapping it in the now-unavailable MP variant.
target_sources_grouped(
  TARGET xrGame
  NAME "ServerEntities_Batch5"
  FILES
  ../xrServerEntities/character_info.cpp
  ../xrServerEntities/object_factory_register.cpp
  ../xrServerEntities/specific_character.cpp
)

# lua_studio.cpp - deliberately excluded, same class of decision as
# script_debugger.cpp above. It's the real client-side glue for the
# "Lua Studio" third-party Lua IDE/debugger: dynamically LoadLibrary()s an
# external plugin DLL and implements `cs::lua_studio::backend`'s
# introspection interface against this fork's own luabind internals
# (`class_rep::properties()`/`gettable()`, `object_rep::get_lua_table()`,
# `luabind::internal_vector`, `detail::is_class_object`, `icon_type_*` -
# the same now-removed old-luabind-internals API surface as §45's
# script_engine_help.cpp, needing a real rewrite, not a bug fix). Real
# code, but every consumer of it (`CScriptEngine::m_lua_studio_world`/
# `debugger()`'s `cs::lua_studio::world*`-returning overload,
# `initialize_lua_studio()`/`finalize_lua_studio()`) is guarded by
# `#ifdef USE_LUA_STUDIO`, which - exactly like `script_debugger.cpp`'s
# `USE_DEBUGGER` - is defined only under `#ifndef MASTER_GOLD` in
# script_storage.h, and MASTER_GOLD is always defined in this build (see
# batch 4's script_debugger.cpp comment for the xrCore.h ordering bug
# that causes this). Permanently unreachable here; even if reachability
# were somehow restored, it depends on a Windows-only third-party IDE
# plugin DLL that has no meaning on Linux anyway. Excluding it costs
# nothing at link time.
#
# smart_cast_stats.cpp - roadmap step 4 (2026-08-30). The single-file
# holdout from ServerEntities_Batch1's exclusion note above: its entire
# body was commented out (even its own #ifdef guards), but
# console_commands.cpp's #ifdef DEBUG-only debug console commands still
# extern-declare and call show_smart_cast_stats()/clear_smart_cast_stats()
# unconditionally - a link-time landmine confirmed to exist in the real
# upstream engine too (smart_cast_stats.cpp is absent from the real
# xrGame.vcxproj), not something this port introduced. Fixed by
# resurrecting just the file's own already-written "SMART_CAST_STATS is
# not defined" fallback branch as real, uncommented code - not restoring
# the disabled stats-tracking feature itself, which stays permanently off
# exactly as upstream left it.
target_sources_grouped(
  TARGET xrGame
  NAME "ServerEntities_Batch6"
  FILES
  ../xrServerEntities/smart_cast_stats.cpp
)

# ScriptXMLInit.cpp (2026-08-30): un-excluded from the root-sweep sixth-
# batch MP list above - see that comment for the triage detail (one dead
# CServerList/ui\ServerList.h touchpoint removed, everything else here is
# real, needed generic XML/UI-init scripting glue, e.g. XMLLuaCallback).
# ui/UIMotionIcon.cpp (2026-08-30): wired in separately, alongside the
# other ui/ files above - its own dead #include "../Actor.cpp" (working
# around a free function with no header declaration) was replaced with a
# plain extern declaration instead.
target_sources_grouped(
  TARGET xrGame
  NAME "Game4_RootBatch16"
  FILES
  ScriptXMLInit.cpp
)

target_link_libraries(xrGame PUBLIC xrCore xrCDB xrSound xrEngine xrPhysics xrNetServer lua-extensions)
