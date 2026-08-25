#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"

void IInputReceiver::IR_Capture()
{
	VERIFY(pInput);
	pInput->iCapture(this);
}

void IInputReceiver::IR_Release()
{
	VERIFY(pInput);
	pInput->iRelease(this);
}

void IInputReceiver::IR_GetLastMouseDelta(Ivector2& p)
{
	VERIFY(pInput);
	pInput->iGetLastMouseDelta(p);
}

void IInputReceiver::IR_OnDeactivate(void)
{
	int i;
	for (i = 0; i < CInput::COUNT_KB_BUTTONS; i++)
		if (IR_GetKeyState(i))
			IR_OnKeyboardRelease(i);

	for (i = 0; i < CInput::COUNT_MOUSE_BUTTONS; i++)
		if (IR_GetBtnState(i))
			IR_OnMouseRelease(i);
	IR_OnMouseStop(DIMOFS_X, 0);
	IR_OnMouseStop(DIMOFS_Y, 0);
}

void IInputReceiver::IR_OnActivate(void)
{
}

BOOL IInputReceiver::IR_GetKeyState(int dik)
{
	VERIFY(pInput);
	return pInput->iGetAsyncKeyState(dik);
}

BOOL IInputReceiver::IR_GetBtnState(int btn)
{
	VERIFY(pInput);
	return pInput->iGetAsyncBtnState(btn);
}

void IInputReceiver::IR_GetMousePosScreen(Ivector2& p)
{
	// Real screen-space cursor position - direct SDL2 equivalent of
	// GetCursorPos(). SDL_GetGlobalMouseState() is available on every
	// desktop platform (X11 included) - the "not available everywhere"
	// caveat OpenXRay's own xr_input.h guards for
	// (SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE, false on Emscripten/Android/
	// iOS/AmigaOS) doesn't apply to this Linux/X11 port.
	SDL_GetGlobalMouseState(&p.x, &p.y);
}

void IInputReceiver::IR_GetMousePosReal(HWND hwnd, Ivector2& p)
{
	IR_GetMousePosScreen(p);
	// Replaces ScreenToClient(hwnd, ...): SDL2 has no direct
	// screen->window-local conversion call, so subtract the window's own
	// screen position by hand (hwnd here is really an SDL_Window*, see
	// device.h - HWND is just this codebase's long-standing opaque
	// "platform window handle" typedef, kept as the parameter type so
	// this signature didn't need to change).
	if (hwnd)
	{
		SDL_Window* window = static_cast<SDL_Window*>(hwnd);
		int wx = 0, wy = 0;
		SDL_GetWindowPosition(window, &wx, &wy);
		p.x -= wx;
		p.y -= wy;
	}
}

void IInputReceiver::IR_GetMousePosReal(Ivector2& p)
{
	IR_GetMousePosReal(RDEVICE.m_sdlWnd, p);
}

void IInputReceiver::IR_GetMousePosIndependent(Fvector2& f)
{
	Ivector2 p;
	IR_GetMousePosReal(p);
	f.set(
		2.f * float(p.x) / float(RDEVICE.clientWidth) - 1.f,
		2.f * float(p.y) / float(RDEVICE.clientHeight) - 1.f
	);
}

void IInputReceiver::IR_GetMousePosIndependentCrop(Fvector2& f)
{
	IR_GetMousePosIndependent(f);
	if (f.x < -1.f) f.x = -1.f;
	if (f.x > 1.f) f.x = 1.f;
	if (f.y < -1.f) f.y = -1.f;
	if (f.y > 1.f) f.y = 1.f;
}
