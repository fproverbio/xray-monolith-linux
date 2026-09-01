// Storage for the renderer-tunable console variables that xrEngine/xrGame
// code reads directly by name (not just through the console), whose real
// definitions normally live in Layers/xrRender/xrRender_console.cpp - a
// file that isn't part of this port's build (it also defines dozens of
// other ps_ssfx_*/ps_r_* tuning vars that genuinely are renderer-tier-only,
// and pulls in dxRenderDeviceRender.h for two of its console-command
// callbacks, so pulling the whole file in here would drag in the real DX11
// renderer implementation it's meant to stand in for). This file supplies
// just the subset actually referenced outside Layers/xrRender - same
// defaults as xrRender_console.cpp - so those callers link. No console
// commands are registered for them here; the vulkan-port stub has no
// console-command registration of its own for the renderer tier.
//
// r_optimize_calculate_bones/wallmark_range_static/wallmark_range_skeleton
// are a separate case: on a real renderer tier their storage lives in
// Layers/xrRender/SkeletonRigid.cpp and WallmarksEngine.cpp respectively
// (not xrRender_console.cpp), with console_commands.cpp reaching them via
// its own local `extern` declarations - no header declares them, so this
// file just matches that pattern directly.

#include "stdafx.h"

// xrRender_console.h's declarations use ECORE_API and ENGINE_API, both
// empty macros defined by xrEngine/stdafx.h (and every other module that
// pulls this header in normally) - not by xrAPI's own stdafx.h, since this
// module never needed them before now.
#define ECORE_API
#define ENGINE_API
#include "../xrRender/xrRender_console.h"

Flags32 psDeviceFlags2 = { 0 };
Flags32 ps_actor_shadow_flags = { 0 };

Fvector4 ps_ssfx_wind_trees = { 11.0f, 0.15f, 0.5f, 0.15f };
Fvector4 ps_ssfx_grass_interactive = { .0f, .0f, 2000.0f, 1.0f };
Fvector4 ps_ssfx_int_grass_params_1 = { 1.0f, 1.0f, 1.0f, 25.0f };
Fvector4 ps_ssfx_int_grass_params_2 = { 1.0f, 5.0f, 1.0f, 1.0f };

float hud_fov_aim_factor = 0;

float sil_glow_max_temp = 0.15f;
float sil_glow_shot_temp = 0.004f;
float sil_glow_cool_temp_rate = 0.01f;

int ps_r2_heatvision = 0;
int heat_vision_cooldown = 1;
float heat_vision_cooldown_time = 20000.f;
int heat_vision_zombie_cold = 0;

int ps_r4_hdr10_pda = 0;

BOOL r_optimize_calculate_bones = TRUE;
float wallmark_range_static = 100.f;
float wallmark_range_skeleton = 50.f;
