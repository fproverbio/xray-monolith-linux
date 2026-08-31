#pragma once

struct xr_token;

// HMONITOR encoding shared between MonitorList.cpp and device.cpp's
// GetStartupMonitor()/InitMonitor()/GetMonitorResolution() family: a
// synthetic id (SDL display index + 1) reinterpret_cast into the opaque
// void*-typed HMONITOR, so 0/nullptr keeps meaning NULL/"Auto"/unset - see
// MonitorList.cpp's file comment for the full rationale.
inline HMONITOR xr_MonitorFromDisplayIndex(int index)
{
	return reinterpret_cast<HMONITOR>(static_cast<intptr_t>(index + 1));
}

inline int xr_DisplayIndexFromMonitor(HMONITOR h)
{
	return static_cast<int>(reinterpret_cast<intptr_t>(h)) - 1;
}

ENGINE_API extern xr_token* vid_monitor_token;
ENGINE_API extern xr_string  vid_monitor_name;

ENGINE_API void     fill_vid_monitor_list();
ENGINE_API void     free_vid_monitor_list();
ENGINE_API void     refresh_vid_monitor_list();
ENGINE_API HMONITOR ResolveSelectedMonitor();

ENGINE_API extern volatile LONG g_monitor_list_dirty;

ENGINE_API HMONITOR GetStartupMonitor();
ENGINE_API void     SetStartupMonitor(HMONITOR h);
ENGINE_API void     ResetStartupMonitor();
