#include "stdafx.h"

#include "../Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "XR_IOConsole.h"
#include "MonitorList.h"

// IGame_Persistent.h is not ported yet (see device.cpp's file comment for
// the full reasoning) - Environment()/bNeed_re_create_env below need its
// complete type, so those specific lines are deferred with a TODO instead
// of pulling the header in.
class IGame_Persistent;
extern IGame_Persistent* g_pGamePersistent;

void CRenderDevice::_Destroy(BOOL bKeepTextures)
{
	DU->OnDeviceDestroy();

	// before destroy
	b_is_Ready = FALSE;
	Statistic->OnDeviceDestroy();
	m_imgui.OnDeviceDestroy();
	::Render->destroy();
	m_pRender->OnDeviceDestroy(bKeepTextures);
	//Resources->OnDeviceDestroy (bKeepTextures);
	//RCache.OnDeviceDestroy ();

	Memory.mem_compact();
}

void CRenderDevice::Destroy(void)
{
	if (!b_is_Ready) return;

	Log("Destroying Direct3D...");

	SDL_ShowCursor(SDL_ENABLE);
	if (m_sdlWnd)
		SDL_SetWindowGrab(m_sdlWnd, SDL_FALSE);
	m_pRender->ValidateHW();

	_Destroy(FALSE);

	// real destroy
	m_pRender->DestroyHW();

	//xr_delete (Resources);
	//HW.DestroyDevice ();

	seqRender.R.clear();
	seqAppActivate.R.clear();
	seqAppDeactivate.R.clear();
	seqAppStart.R.clear();
	seqAppEnd.R.clear();
	seqFrame.R.clear();
	seqFrameMT.R.clear();
	seqDeviceReset.R.clear();
	seqParallel.clear();

	RenderFactory->DestroyRenderDeviceRender(m_pRender);
	m_pRender = 0;
	xr_delete(Statistic);
}

#include "CustomHUD.h"
extern bool use_reshade;
extern bool init_reshade();
extern void unregister_reshade();
extern u32 g_screenmode;
extern void GetMonitorResolution(u32& horizontal, u32& vertical);
extern void GetMonitorPosition(int& x, int& y);
extern ENGINE_API u32 psCurrentVidMode[];

void CRenderDevice::Reset(bool precache)
{
	if (use_reshade)
		unregister_reshade();

	use_reshade = false;

	m_imgui.OnDeviceResetBegin();

	u32 dwWidth_before = dwWidth;
	u32 dwHeight_before = dwHeight;

	SDL_ShowCursor(SDL_ENABLE);
	u32 tm_start = TimerAsync();

	m_pRender->Reset(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

	// TODO: g_pGamePersistent->Environment().bNeed_re_create_env needs
	// IGame_Persistent.h/Environment.h, neither ported yet (see the
	// class-forward-declaration comment above) - deferred.

	_SetupStates();
	if (precache)
		PreCache(20, true, false);
	u32 tm_end = TimerAsync();
	Msg("*** RESET [%d ms]", tm_end - tm_start);

	// TODO: Remove this! It may hide crash
	Memory.mem_compact();

	seqDeviceReset.Process(rp_DeviceReset);

	if (dwWidth_before != dwWidth || dwHeight_before != dwHeight)
	{
		seqResolutionChanged.Process(rp_ScreenResolutionChanged);
	}

	if (g_screenmode == 1)
	{
		u32 w, h;
		int monX, monY;
		GetMonitorResolution(w, h);
		GetMonitorPosition(monX, monY);
		// Replaces SetWindowLongPtr(..., WS_VISIBLE | WS_POPUP) +
		// SetWindowPos(..., SWP_FRAMECHANGED) - same
		// borderless-fill-the-monitor shape as Device_create.cpp's
		// g_screenmode==0 branch, just also explicitly (re)shown.
		SDL_SetWindowBordered(Device.m_sdlWnd, SDL_FALSE);
		SDL_SetWindowSize(Device.m_sdlWnd, static_cast<int>(w), static_cast<int>(h));
		SDL_SetWindowPosition(Device.m_sdlWnd, monX, monY);
		SDL_ShowWindow(Device.m_sdlWnd);
	}

#ifndef DEDICATED_SERVER
	SDL_ShowCursor(SDL_DISABLE);
	int cw = 0, ch = 0;
	SDL_GetWindowSize(m_sdlWnd, &cw, &ch);
	clientWidth = static_cast<u32>(cw);
	clientHeight = static_cast<u32>(ch);
	if (m_sdlWnd)
		SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);
#endif

	m_imgui.OnDeviceResetEnd();
	use_reshade = init_reshade();
}

bool CRenderDevice::ChangeOutputMonitor(HMONITOR hTargetMon)
{
	static bool s_swap_in_progress = false;
	if (!b_is_Ready)
		return false;
	if (s_swap_in_progress)
		return false;
	if (m_sdlWnd && (SDL_GetWindowFlags(m_sdlWnd) & SDL_WINDOW_MINIMIZED))
	{
		Msg("* vid_monitor: window is minimised, deferring monitor switch to restart");
		return false;
	}

	if (hTargetMon == GetStartupMonitor())
	{
		Msg("* vid_monitor: already on target monitor, no-op");
		return true;
	}

	struct SwapGuard
	{
		bool& flag;
		SwapGuard(bool& f) : flag(f) { flag = true; }
		~SwapGuard() { flag = false; }
	} guard(s_swap_in_progress);

	if (use_reshade)
		unregister_reshade();
	use_reshade = false;

	m_imgui.OnDeviceResetBegin();
	SDL_ShowCursor(SDL_ENABLE);

	u32 dwWidth_before  = dwWidth;
	u32 dwHeight_before = dwHeight;

	bool switched = m_pRender->SwitchOutputMonitor(hTargetMon, m_sdlWnd,
	                                               g_screenmode,
	                                               psCurrentVidMode[0], psCurrentVidMode[1]);

	if (!switched)
	{
		m_imgui.OnDeviceResetEnd();
		SDL_ShowCursor(SDL_DISABLE);
		use_reshade = init_reshade();
		return false;
	}

	m_pRender->Reset(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

	// TODO: g_pGamePersistent->Environment().bNeed_re_create_env needs
	// IGame_Persistent.h/Environment.h, neither ported yet (see the
	// class-forward-declaration comment above) - deferred.

	_SetupStates();
	PreCache(20, true, false);

	seqDeviceReset.Process(rp_DeviceReset);

	if (dwWidth_before != dwWidth || dwHeight_before != dwHeight)
		seqResolutionChanged.Process(rp_ScreenResolutionChanged);

#ifndef DEDICATED_SERVER
	SDL_ShowCursor(SDL_DISABLE);
	int cw = 0, ch = 0;
	SDL_GetWindowSize(m_sdlWnd, &cw, &ch);
	clientWidth  = static_cast<u32>(cw);
	clientHeight = static_cast<u32>(ch);
	if (m_sdlWnd)
		SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);
#endif

	m_imgui.OnDeviceResetEnd();
	use_reshade = init_reshade();

	Msg("* vid_monitor: live switch succeeded");
	return true;
}
