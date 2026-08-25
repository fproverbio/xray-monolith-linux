#include "stdafx.h"

//#include "resourcemanager.h"
#include "../Include/xrRender/DrawUtils.h"
//#include "xr_effgamma.h"
#include "Render.h"
#include "dedicated_server_only.h"
#include "../xrCDB/xrXRC.h"

//#include "securom_api.h"

extern XRCDB_API BOOL* cdb_bDebug;

void SetupGPU(IRenderDeviceRender* pRender)
{
	// Command line
	char* lpCmdLine = Core.Params;

	BOOL bForceGPU_SW;
	BOOL bForceGPU_NonPure;
	BOOL bForceGPU_REF;

	if (strstr(lpCmdLine, "-gpu_sw") != NULL) bForceGPU_SW = TRUE;
	else bForceGPU_SW = FALSE;
	if (strstr(lpCmdLine, "-gpu_nopure") != NULL) bForceGPU_NonPure = TRUE;
	else bForceGPU_NonPure = FALSE;
	if (strstr(lpCmdLine, "-gpu_ref") != NULL) bForceGPU_REF = TRUE;
	else bForceGPU_REF = FALSE;

	pRender->SetupGPU(bForceGPU_SW, bForceGPU_NonPure, bForceGPU_REF);
}

void CRenderDevice::_SetupStates()
{
	// General Render States
	mView.identity();
	mProject.identity();
	mFullTransform.identity();
	vCameraPosition.set(0, 0, 0);
	vCameraDirection.set(0, 0, 1);
	vCameraTop.set(0, 1, 0);
	vCameraRight.set(1, 0, 0);

	m_pRender->SetupStates();
}

void CRenderDevice::_Create(LPCSTR shName)
{
	Memory.mem_compact();

	// after creation
	b_is_Ready = TRUE;
	_SetupStates();

	m_pRender->OnDeviceCreate(shName);
	m_imgui.OnDeviceCreate();
	dwFrame = 0;
}

void CRenderDevice::ConnectToRender()
{
	if (!m_pRender)
		m_pRender = RenderFactory->CreateRenderDeviceRender();
}

extern u32 g_screenmode;
extern void GetMonitorResolution(u32& horizontal, u32& vertical);
extern void GetMonitorPosition(int& x, int& y);

PROTECT_API void CRenderDevice::Create()
{
	//SECUROM_MARKER_SECURITY_ON(4)

	if (b_is_Ready) return; // prevent double call

	u32 w, h;
	GetMonitorResolution(w, h);
	int monX, monY;
	GetMonitorPosition(monX, monY);
	if (psCurrentVidMode[0] == 0 || psCurrentVidMode[1] == 0)
	{
		psCurrentVidMode[0] = w;
		psCurrentVidMode[1] = h;
	}

	// Replaces SetWindowLongPtr(m_hWnd, GWL_STYLE, style) +
	// SetWindowPos(..., SWP_FRAMECHANGED): SDL2 tracks "has a title
	// bar/border" and size/position as three separate, independent calls
	// rather than one Win32 style bitmask + one geometry call.
	if (g_screenmode == 0)
	{
		SDL_SetWindowBordered(m_sdlWnd, SDL_TRUE);
		w = psCurrentVidMode[0];
		h = psCurrentVidMode[1];
	}
	else
	{
		SDL_SetWindowBordered(m_sdlWnd, SDL_FALSE);
	}
	SDL_SetWindowSize(m_sdlWnd, static_cast<int>(w), static_cast<int>(h));
	SDL_SetWindowPosition(m_sdlWnd, monX, monY);

	Statistic = xr_new<CStats>();
#ifdef DEBUG
    cdb_clRAY = &Statistic->clRAY; // total: ray-testing
    cdb_clBOX = &Statistic->clBOX; // total: box query
    cdb_clFRUSTUM = &Statistic->clFRUSTUM; // total: frustum query
    cdb_bDebug = &bDebug;
#endif

	if (!m_pRender)
		m_pRender = RenderFactory->CreateRenderDeviceRender();
	SetupGPU(m_pRender);
	Log("Starting RENDER device...");

#ifdef _EDITOR
    psCurrentVidMode[0] = dwWidth;
    psCurrentVidMode[1] = dwHeight;
#endif // #ifdef _EDITOR

	fFOV = 90.f;
	fASPECT = 1.f;
	m_pRender->Create(
		m_sdlWnd,
		dwWidth,
		dwHeight,
		fWidth_2,
		fHeight_2,
#ifdef INGAME_EDITOR
        editor() ? false :
#endif // #ifdef INGAME_EDITOR
		true
	);

	// DisableProcessWindowsGhosting(): Win32-only (tells the OS not to
	// paint a translucent "ghost" of the window while it's not pumping
	// messages) - X11/SDL2 has no matching "not responding" overlay
	// concept for this to opt out of.
#ifdef _WIN32
	DisableProcessWindowsGhosting();
#endif

	// Replaces GetClientRect/MapWindowPoints/ClipCursor/SetActiveWindow:
	// query real client size from SDL2, grab the cursor to the window
	// (SDL2's whole-window grab is the closest equivalent to an arbitrary
	// ClipCursor rect - see device.cpp's OnWindowActivate() for the same
	// substitution), and raise/focus the window.
	int cw = 0, ch = 0;
	SDL_GetWindowSize(m_sdlWnd, &cw, &ch);
	clientWidth = static_cast<u32>(cw);
	clientHeight = static_cast<u32>(ch);
	SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);
	SDL_RaiseWindow(m_sdlWnd);

	string_path fname;
	FS.update_path(fname, "$game_data$", "shaders.xr");

	//////////////////////////////////////////////////////////////////////////
	_Create(fname);

	PreCache(0, false, false);

	//SECUROM_MARKER_SECURITY_OFF(4)
}
