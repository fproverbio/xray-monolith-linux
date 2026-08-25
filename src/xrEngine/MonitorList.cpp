// MonitorList.cpp — monitor enumeration for the vid_monitor dropdown.
//
// Originally enumerated real Win32 monitors via EnumDisplayMonitors +
// GetMonitorInfoA, with friendly names looked up via the Windows 7+
// QueryDisplayConfig/DisplayConfigGetDeviceInfo APIs (loaded dynamically)
// plus a PnP "Device description" fallback via cfgmgr32 for monitors
// DisplayConfig doesn't have a friendly name for (typical of internal
// laptop panels). None of that has a Linux/X11 equivalent worth
// reimplementing 1:1 - see playground/xray-monolith-vulkan-port-notes.md,
// the SDL2/Vulkan window+device bootstrap pass. Ported to SDL2's own
// display-query API instead: SDL_GetNumVideoDisplays()/SDL_GetDisplayName()
// already give one friendly name per display directly, no DisplayConfig-
// style two-pass source/target device matching or connector-type suffix
// disambiguation needed.
//
// HMONITOR here is *not* a real Win32 monitor handle (win32_compat.h only
// defines it as an opaque void* stand-in) - it is a small synthetic id,
// the SDL display index plus one, reinterpret_cast into that void*. The
// "+1" keeps 0 (== nullptr) free to keep meaning NULL/"Auto"/unset, which
// is the exact convention every existing call site (ResolveSelectedMonitor
// returning NULL for "Auto", device.cpp's GetStartupMonitor()/
// InitMonitor()) already assumes.

#include "stdafx.h"
#include "MonitorList.h"

#include <SDL.h>

ENGINE_API xr_token* vid_monitor_token = nullptr;
ENGINE_API xr_string vid_monitor_name = "Auto";
ENGINE_API volatile long g_monitor_list_dirty = 0;

void fill_vid_monitor_list()
{
	if (vid_monitor_token != nullptr)
		return;

	int N = SDL_GetNumVideoDisplays();
	if (N < 0)
	{
		Msg("! vid_monitor: SDL_GetNumVideoDisplays failed (%s), list will be Auto-only", SDL_GetError());
		N = 0;
	}

	vid_monitor_token = xr_alloc<xr_token>(static_cast<u32>(N) + 2);

	vid_monitor_token[0].name = xr_strdup("Auto");
	vid_monitor_token[0].id = 0;

	for (int i = 0; i < N; ++i)
	{
		const char* name = SDL_GetDisplayName(i);
		string64 fallback;
		if (!name || !name[0])
		{
			xr_sprintf(fallback, sizeof(fallback), "Display %d", i + 1);
			name = fallback;
		}

		vid_monitor_token[i + 1].name = xr_strdup(name);
		vid_monitor_token[i + 1].id = i + 1;
	}

	vid_monitor_token[N + 1].name = nullptr;
	vid_monitor_token[N + 1].id = -1;

	Msg("* vid_monitor: enumerated %d monitor(s)", N);
}

void free_vid_monitor_list()
{
	if (!vid_monitor_token)
		return;

	for (int i = 0; vid_monitor_token[i].name; ++i)
		xr_free(vid_monitor_token[i].name);

	xr_free(vid_monitor_token);
	vid_monitor_token = nullptr;
}

HMONITOR ResolveSelectedMonitor()
{
	const xr_string& name = vid_monitor_name;

	if (name.empty() || name == "Auto")
		return nullptr;

	if (!vid_monitor_token)
		return nullptr;

	for (int i = 1; vid_monitor_token[i].name; ++i)
	{
		if (xr_strcmp(vid_monitor_token[i].name, name.c_str()) == 0)
			return xr_MonitorFromDisplayIndex(i - 1); // token[i] is display index (i-1)
	}

	Msg("! vid_monitor: '%s' not found on this system, using Auto", name.c_str());
	return nullptr;
}

void refresh_vid_monitor_list()
{
	Msg("* vid_monitor: live-refresh triggered");
	free_vid_monitor_list();
	fill_vid_monitor_list();
}
