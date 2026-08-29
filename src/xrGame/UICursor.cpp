#include "StdAfx.h"
#include "UICursor.h"

#include "ui/UIStatic.h"
#include "ui/UIBtnHint.h"
#include "IInputReceiver.h"

#define C_DEFAULT	D3DCOLOR_XRGB(0xff,0xff,0xff)

// Established local-extern pattern for this function throughout xrEngine
// (Device_create.cpp/Device_destroy.cpp/x_ray.cpp/etc.) - real SDL-backed
// implementation lives in xrEngine/device.cpp, no shared header declares it.
extern void GetMonitorResolution(u32& horizontal, u32& vertical);

CUICursor::CUICursor()
	: m_static(NULL), m_b_use_win_cursor(false)
{
	bVisible = false;
	vPrevPos.set(0.0f, 0.0f);
	vPos.set(0.f, 0.f);
	InitInternal();
	Device.seqRender.Add(this, -3/*2*/);
	Device.seqResolutionChanged.Add(this);
}

//--------------------------------------------------------------------
CUICursor::~CUICursor()
{
	xr_delete(m_static);
	Device.seqRender.Remove(this);
	Device.seqResolutionChanged.Remove(this);
}

void CUICursor::OnScreenResolutionChanged()
{
	xr_delete(m_static);
	InitInternal();
}

void CUICursor::InitInternal()
{
	m_static = xr_new<CUIStatic>();
	m_static->InitTextureEx("ui\\ui_ani_cursor", "hud\\cursor");
	Frect rect;
	rect.set(0.0f, 0.0f, 40.0f, 40.0f);
	m_static->SetTextureRect(rect);
	Fvector2 sz;
	sz.set(rect.rb);
	sz.x *= UI().get_current_kx();

	m_static->SetWndSize(sz);
	m_static->SetStretchTexture(true);

	u32 screen_size_x, screen_size_y;
	GetMonitorResolution(screen_size_x, screen_size_y);
	m_b_use_win_cursor = (screen_size_y >= Device.dwHeight && screen_size_x >= Device.dwWidth);
}

//--------------------------------------------------------------------
u32 last_render_frame = 0;

void CUICursor::OnRender()
{
	g_btnHint->OnRender();
	g_statHint->OnRender();

	if (!IsVisible()) return;
#ifdef DEBUG
	VERIFY(last_render_frame != Device.dwFrame);
	last_render_frame = Device.dwFrame;

	if(bDebug)
	{
	CGameFont* F		= UI().Font().pFontDI;
	F->SetAligment		(CGameFont::alCenter);
	F->SetHeightI		(0.02f);
	F->OutSetI			(0.f,-0.9f);
	F->SetColor			(0xffffffff);
	Fvector2			pt = GetCursorPosition();
	F->OutNext			("%f-%f",pt.x, pt.y);
	}
#endif

	u32 curFrame = Device.dwFrame;
	if (curFrame == last_render_frame)
		return;

	m_static->SetWndPos(vPos);
	m_static->Update();
	m_static->Draw();

	last_render_frame = curFrame;
}

Fvector2 CUICursor::GetCursorPosition()
{
	return vPos;
}

Fvector2 CUICursor::GetCursorPositionDelta()
{
	Fvector2 res_delta;

	res_delta.x = vPos.x - vPrevPos.x;
	res_delta.y = vPos.y - vPrevPos.y;
	return res_delta;
}

void CUICursor::UpdateCursorPosition(int _dx, int _dy)
{
	Fvector2 p;
	vPrevPos = vPos;
	if (m_b_use_win_cursor)
	{
		Ivector2 pti;
		IInputReceiver::IR_GetMousePosReal(pti);
		p.x = (float)pti.x;
		p.y = (float)pti.y;
		vPos.x = p.x * (UI_BASE_WIDTH / (float)Device.clientWidth);
		vPos.y = p.y * (UI_BASE_HEIGHT / (float)Device.clientHeight);
	}
	else
	{
		float sens = 1.0f;
		vPos.x += _dx * sens;
		vPos.y += _dy * sens;
	}
	clamp(vPos.x, 0.f, UI_BASE_WIDTH);
	clamp(vPos.y, 0.f, UI_BASE_HEIGHT);
}

void CUICursor::SetUICursorPosition(Fvector2 pos)
{
	vPos = pos;
	int x = iFloor(vPos.x / (UI_BASE_WIDTH / (float)Device.clientWidth));
	int y = iFloor(vPos.y / (UI_BASE_HEIGHT / (float)Device.clientHeight));
	// ClientToScreen+SetCursorPos was the Win32 way to warp the OS cursor to
	// a window-client-space point; SDL_WarpMouseInWindow already takes
	// client-space coordinates directly (see imgui_base.cpp's identical
	// use), so no separate client->screen conversion step is needed here.
	SDL_WarpMouseInWindow(Device.m_sdlWnd, x, y);
}
