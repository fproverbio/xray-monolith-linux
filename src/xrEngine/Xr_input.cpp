#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
//#include "../include/editor/ide.hpp"

#ifndef _EDITOR
# include "xr_input_xinput.h"
#endif
CInput* pInput = NULL;
IInputReceiver dummyController;

ENGINE_API float psMouseSens = 1.f;
ENGINE_API float psMouseSensScale = 1.f;
ENGINE_API float psMouseSensVerticalK = 1.f;
ENGINE_API Flags32 psMouseInvert = {FALSE};

float stop_vibration_time = flt_max;

// demonized: configurable buffer sizes for mouse and kb
int MOUSEBUFFERSIZE = 1024;
int KEYBOARDBUFFERSIZE = 128;

#define _KEYDOWN(name,key) ( name[key] & 0x80 )

static bool g_exclusive = false;
extern u32 g_screenmode;

static SDL_Window* get_game_window()
{
	return Device.m_sdlWnd;
}

static void on_error_dialog(bool before)
{
#ifdef INGAME_EDITOR
    if (Device.editor())
        return;
#endif // #ifdef INGAME_EDITOR
	if (!pInput || !g_exclusive)
		return;

	if (before)
	{
		pInput->unacquire();
		return;
	}

	pInput->acquire(true);
}

// --- DIK_* <-> SDL_Scancode mapping ---------------------------------------
//
// Real input now goes through SDL2's polling API (SDL_GetKeyboardState/
// SDL_GetMouseState/SDL_GetRelativeMouseState - see KeyUpdate()/
// MouseUpdate() below), replacing DirectInput device polling. xr_input.h's
// public API (CInput::iGetAsyncKeyState(), IInputReceiver::IR_OnKeyboard*,
// imgui_helper.h's whole DIK_*->ImGuiKey_* table) is left exactly as-is -
// still DIK_*-scancode-shaped - since nothing about that shape is actually
// DirectInput-specific (it's just a numbering scheme) and every existing
// consumer of CInput already expects it (same "preserve the declared
// shape, adapt only the implementation underneath" principle already
// established for xrNetServer's DirectPlay8 removal - see
// playground/xray-monolith-vulkan-port-notes.md). So: SDL2 does the real
// polling, this table translates SDL_Scancode -> DIK_* once per key, and
// every call site above KeyUpdate()/MouseUpdate() keeps talking in DIK_*
// exactly as before.
static SDL_Scancode DikToScancode(int dik)
{
	switch (dik)
	{
	case DIK_ESCAPE: return SDL_SCANCODE_ESCAPE;
	case DIK_1: return SDL_SCANCODE_1;
	case DIK_2: return SDL_SCANCODE_2;
	case DIK_3: return SDL_SCANCODE_3;
	case DIK_4: return SDL_SCANCODE_4;
	case DIK_5: return SDL_SCANCODE_5;
	case DIK_6: return SDL_SCANCODE_6;
	case DIK_7: return SDL_SCANCODE_7;
	case DIK_8: return SDL_SCANCODE_8;
	case DIK_9: return SDL_SCANCODE_9;
	case DIK_0: return SDL_SCANCODE_0;
	case DIK_MINUS: return SDL_SCANCODE_MINUS;
	case DIK_EQUALS: return SDL_SCANCODE_EQUALS;
	case DIK_BACK: return SDL_SCANCODE_BACKSPACE;
	case DIK_TAB: return SDL_SCANCODE_TAB;
	case DIK_Q: return SDL_SCANCODE_Q;
	case DIK_W: return SDL_SCANCODE_W;
	case DIK_E: return SDL_SCANCODE_E;
	case DIK_R: return SDL_SCANCODE_R;
	case DIK_T: return SDL_SCANCODE_T;
	case DIK_Y: return SDL_SCANCODE_Y;
	case DIK_U: return SDL_SCANCODE_U;
	case DIK_I: return SDL_SCANCODE_I;
	case DIK_O: return SDL_SCANCODE_O;
	case DIK_P: return SDL_SCANCODE_P;
	case DIK_LBRACKET: return SDL_SCANCODE_LEFTBRACKET;
	case DIK_RBRACKET: return SDL_SCANCODE_RIGHTBRACKET;
	case DIK_RETURN: return SDL_SCANCODE_RETURN;
	case DIK_LCONTROL: return SDL_SCANCODE_LCTRL;
	case DIK_A: return SDL_SCANCODE_A;
	case DIK_S: return SDL_SCANCODE_S;
	case DIK_D: return SDL_SCANCODE_D;
	case DIK_F: return SDL_SCANCODE_F;
	case DIK_G: return SDL_SCANCODE_G;
	case DIK_H: return SDL_SCANCODE_H;
	case DIK_J: return SDL_SCANCODE_J;
	case DIK_K: return SDL_SCANCODE_K;
	case DIK_L: return SDL_SCANCODE_L;
	case DIK_SEMICOLON: return SDL_SCANCODE_SEMICOLON;
	case DIK_APOSTROPHE: return SDL_SCANCODE_APOSTROPHE;
	case DIK_GRAVE: return SDL_SCANCODE_GRAVE;
	case DIK_LSHIFT: return SDL_SCANCODE_LSHIFT;
	case DIK_BACKSLASH: return SDL_SCANCODE_BACKSLASH;
	case DIK_Z: return SDL_SCANCODE_Z;
	case DIK_X: return SDL_SCANCODE_X;
	case DIK_C: return SDL_SCANCODE_C;
	case DIK_V: return SDL_SCANCODE_V;
	case DIK_B: return SDL_SCANCODE_B;
	case DIK_N: return SDL_SCANCODE_N;
	case DIK_M: return SDL_SCANCODE_M;
	case DIK_COMMA: return SDL_SCANCODE_COMMA;
	case DIK_PERIOD: return SDL_SCANCODE_PERIOD;
	case DIK_SLASH: return SDL_SCANCODE_SLASH;
	case DIK_RSHIFT: return SDL_SCANCODE_RSHIFT;
	case DIK_MULTIPLY: return SDL_SCANCODE_KP_MULTIPLY;
	case DIK_LMENU: return SDL_SCANCODE_LALT;
	case DIK_SPACE: return SDL_SCANCODE_SPACE;
	case DIK_CAPITAL: return SDL_SCANCODE_CAPSLOCK;
	case DIK_F1: return SDL_SCANCODE_F1;
	case DIK_F2: return SDL_SCANCODE_F2;
	case DIK_F3: return SDL_SCANCODE_F3;
	case DIK_F4: return SDL_SCANCODE_F4;
	case DIK_F5: return SDL_SCANCODE_F5;
	case DIK_F6: return SDL_SCANCODE_F6;
	case DIK_F7: return SDL_SCANCODE_F7;
	case DIK_F8: return SDL_SCANCODE_F8;
	case DIK_F9: return SDL_SCANCODE_F9;
	case DIK_F10: return SDL_SCANCODE_F10;
	case DIK_NUMLOCK: return SDL_SCANCODE_NUMLOCKCLEAR;
	case DIK_SCROLL: return SDL_SCANCODE_SCROLLLOCK;
	case DIK_NUMPAD7: return SDL_SCANCODE_KP_7;
	case DIK_NUMPAD8: return SDL_SCANCODE_KP_8;
	case DIK_NUMPAD9: return SDL_SCANCODE_KP_9;
	case DIK_SUBTRACT: return SDL_SCANCODE_KP_MINUS;
	case DIK_NUMPAD4: return SDL_SCANCODE_KP_4;
	case DIK_NUMPAD5: return SDL_SCANCODE_KP_5;
	case DIK_NUMPAD6: return SDL_SCANCODE_KP_6;
	case DIK_ADD: return SDL_SCANCODE_KP_PLUS;
	case DIK_NUMPAD1: return SDL_SCANCODE_KP_1;
	case DIK_NUMPAD2: return SDL_SCANCODE_KP_2;
	case DIK_NUMPAD3: return SDL_SCANCODE_KP_3;
	case DIK_NUMPAD0: return SDL_SCANCODE_KP_0;
	case DIK_DECIMAL: return SDL_SCANCODE_KP_PERIOD;
	case DIK_OEM_102: return SDL_SCANCODE_NONUSBACKSLASH;
	case DIK_F11: return SDL_SCANCODE_F11;
	case DIK_F12: return SDL_SCANCODE_F12;
	case DIK_NUMPADEQUALS: return SDL_SCANCODE_KP_EQUALS;
	case DIK_NUMPADENTER: return SDL_SCANCODE_KP_ENTER;
	case DIK_RCONTROL: return SDL_SCANCODE_RCTRL;
	case DIK_NUMPADCOMMA: return SDL_SCANCODE_KP_COMMA;
	case DIK_DIVIDE: return SDL_SCANCODE_KP_DIVIDE;
	case DIK_SYSRQ: return SDL_SCANCODE_PRINTSCREEN;
	case DIK_RMENU: return SDL_SCANCODE_RALT;
	case DIK_PAUSE: return SDL_SCANCODE_PAUSE;
	case DIK_HOME: return SDL_SCANCODE_HOME;
	case DIK_UP: return SDL_SCANCODE_UP;
	case DIK_PRIOR: return SDL_SCANCODE_PAGEUP;
	case DIK_LEFT: return SDL_SCANCODE_LEFT;
	case DIK_RIGHT: return SDL_SCANCODE_RIGHT;
	case DIK_END: return SDL_SCANCODE_END;
	case DIK_DOWN: return SDL_SCANCODE_DOWN;
	case DIK_NEXT: return SDL_SCANCODE_PAGEDOWN;
	case DIK_INSERT: return SDL_SCANCODE_INSERT;
	case DIK_DELETE: return SDL_SCANCODE_DELETE;
	case DIK_LWIN: return SDL_SCANCODE_LGUI;
	case DIK_RWIN: return SDL_SCANCODE_RGUI;
	case DIK_APPS: return SDL_SCANCODE_APPLICATION;
	default: return SDL_SCANCODE_UNKNOWN;
	}
}

CInput::CInput(BOOL bExclusive, int deviceForInit)
{
	g_exclusive = !!bExclusive;

	Log("Starting INPUT device...");

	// pDI/pMouse/pKeyboard (real DirectInput COM interfaces on Windows)
	// have no SDL2 equivalent to hold - SDL2 keyboard/mouse state is
	// queried directly (SDL_GetKeyboardState/SDL_GetMouseState), there is
	// no separate "device object" to create/acquire/release. Left null
	// and unused rather than removed from xr_input.h - they're private,
	// nothing outside this class touches them, and keeping the header
	// unchanged here means one less thing to re-verify (see notes file's
	// "preserve the declared shape" principle).
	pDI = NULL;
	pMouse = NULL;
	pKeyboard = NULL;

	//=====================Mouse
	mouse_property.mouse_dt = 25;

	ZeroMemory(mouseState, sizeof(mouseState));
	ZeroMemory(KBState, sizeof(KBState));
	ZeroMemory(timeStamp, sizeof(timeStamp));
	ZeroMemory(timeSave, sizeof(timeStamp));
	ZeroMemory(offs, sizeof(offs));

	//===================== Dummy pack
	iCapture(&dummyController);

	// deviceForInit (mouse_device_key/keyboard_device_key) no longer picks
	// which DirectInput device object to create - SDL2 keyboard/mouse
	// state is always available once SDL_INIT_VIDEO succeeds, there is no
	// per-device init step left to conditionally skip. Parameter kept for
	// API compatibility (existing call sites still pass it).
	(void)deviceForInit;

	Debug.set_on_dialog(&on_error_dialog);
	Debug.set_window_getter(&get_game_window);

#ifdef ENGINE_BUILD
	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
#endif
}

CInput::~CInput(void)
{
#ifdef ENGINE_BUILD
	Device.seqFrame.Remove(this);
	Device.seqAppDeactivate.Remove(this);
	Device.seqAppActivate.Remove(this);
#endif
	//_______________________
	// No DirectInput device interfaces to unacquire/release under SDL2 -
	// see the constructor's comment on pDI/pMouse/pKeyboard.
}

void CInput::SetAllAcquire(BOOL bAcquire)
{
	SetKBDAcquire(bAcquire);
	SetMouseAcquire(bAcquire);
}

void CInput::SetMouseAcquire(BOOL bAcquire)
{
	// Closest SDL2 equivalent of DirectInput's exclusive mouse
	// acquire/unacquire: relative mode both hides the cursor and reports
	// deltas unbounded by screen edges, which is what "acquired" mouse
	// input meant here in practice (see MouseUpdate()/GrabInput-style
	// call sites elsewhere in this port).
	SDL_SetRelativeMouseMode(bAcquire ? SDL_TRUE : SDL_FALSE);
}

void CInput::SetKBDAcquire(BOOL bAcquire)
{
	// SDL2 keyboard polling has no separate "acquire" step to mirror -
	// SDL_GetKeyboardState() always reflects real keyboard state once the
	// window has focus. No-op kept for API compatibility.
	(void)bAcquire;
}

//-----------------------------------------------------------------------
BOOL b_altF4 = FALSE;

void CInput::KeyUpdate()
{
	if (b_altF4) return;

	static BOOL prevKB[COUNT_KB_BUTTONS] = {};

	const Uint8* sdlState = SDL_GetKeyboardState(nullptr);

	for (int dik = 0; dik < COUNT_KB_BUTTONS; ++dik)
	{
		const SDL_Scancode sc = DikToScancode(dik);
		if (sc == SDL_SCANCODE_UNKNOWN)
			continue;

		KBState[dik] = sdlState[sc] ? TRUE : FALSE;
	}

#ifndef _EDITOR
	bool b_alt_tab = false;

	if (!b_altF4 && KBState[DIK_F4] && (KBState[DIK_RMENU] || KBState[DIK_LMENU]))
	{
		b_altF4 = TRUE;
		Engine.Event.Defer("KERNEL:disconnect");
		Engine.Event.Defer("KERNEL:quit");
	}


#endif
	if (b_altF4) return;

#ifndef _EDITOR
	if (Device.dwPrecacheFrame == 0)
#endif
	{
		for (int dik = 0; dik < COUNT_KB_BUTTONS; ++dik)
		{
			if (DikToScancode(dik) == SDL_SCANCODE_UNKNOWN)
				continue;

			if (KBState[dik] && !prevKB[dik])
				cbStack.back()->IR_OnKeyboardPress(dik);
			else if (!KBState[dik] && prevKB[dik])
			{
				cbStack.back()->IR_OnKeyboardRelease(dik);
#ifndef _EDITOR
				if (dik == DIK_TAB && (iGetAsyncKeyState(DIK_RMENU) || iGetAsyncKeyState(DIK_LMENU)))
					b_alt_tab = true;
#endif
			}
		}

		for (u32 i = 0; i < COUNT_KB_BUTTONS; i++)
			if (KBState[i])
				cbStack.back()->IR_OnKeyboardHold(i);
	}

	memcpy(prevKB, KBState, sizeof(prevKB));

#ifndef _EDITOR
	if (b_alt_tab) {
		BOOL fullscreen = (g_screenmode == 2);
		if (fullscreen)
			SDL_MinimizeWindow(Device.m_sdlWnd);
	}
#endif
	// XInput gamepad polling was already fully commented out upstream in
	// xr_input_xinput.h/.cpp before this port touched the file (dead code,
	// not something this pass disabled) - see xr_input_xinput.cpp.
}

void CInput::resetMouseState()
{
	for (int i = 0; i < COUNT_MOUSE_BUTTONS; i++) {
		mouseState[i] = 0;
	}
}

bool CInput::get_dik_name(int dik, LPSTR dest_str, int dest_sz)
{
	const SDL_Scancode sc = DikToScancode(dik);
	if (sc == SDL_SCANCODE_UNKNOWN)
		return false;

	// SDL_GetKeyName() is layout-aware (unlike DirectInput's
	// DIPROP_KEYNAME, which always returned the US-layout name) - a
	// strict improvement for a "show the user what key this is" label,
	// which is all get_dik_name()'s call sites ever use this for.
	const char* name = SDL_GetKeyName(SDL_GetKeyFromScancode(sc));
	if (!name || !name[0])
		return false;

	xr_strcpy(dest_str, dest_sz, name);
	return true;
}

bool CInput::dik_to_text(int dik, bool shift, bool caps, bool ctrl, bool alt, bool altgr, LPSTR dest, int dest_sz)
{
	if (!dest || dest_sz <= 1)
		return false;

	dest[0] = 0;

	// Ctrl/Alt/AltGr text composition (dead-key-aware Unicode composition
	// via the active keyboard layout) had no portable one-shot equivalent
	// to Win32's ToUnicodeEx() - SDL2's real answer to "what text does
	// this keypress produce" is the SDL_TEXTINPUT event (already wired up
	// in the real input path, see xr_input.h/Xr_input.cpp's event-driven
	// counterpart once ported), not a per-scancode+modifier-state query
	// like this one. Honestly reporting "can't determine" for the
	// modifier-combination cases matches this function's own existing
	// "dead key -> return false" convention rather than guessing.
	if (ctrl || alt || altgr)
		return false;

	const SDL_Scancode sc = DikToScancode(dik);
	if (sc == SDL_SCANCODE_UNKNOWN)
		return false;

	// Common US-QWERTY shifted symbol row - real, standard, but
	// US-layout-specific (documented limitation, not invented data).
	struct ShiftEntry { SDL_Scancode sc; char base; char shifted; };
	static const ShiftEntry kShift[] = {
		{ SDL_SCANCODE_1, '1', '!' }, { SDL_SCANCODE_2, '2', '@' },
		{ SDL_SCANCODE_3, '3', '#' }, { SDL_SCANCODE_4, '4', '$' },
		{ SDL_SCANCODE_5, '5', '%' }, { SDL_SCANCODE_6, '6', '^' },
		{ SDL_SCANCODE_7, '7', '&' }, { SDL_SCANCODE_8, '8', '*' },
		{ SDL_SCANCODE_9, '9', '(' }, { SDL_SCANCODE_0, '0', ')' },
		{ SDL_SCANCODE_MINUS, '-', '_' }, { SDL_SCANCODE_EQUALS, '=', '+' },
		{ SDL_SCANCODE_LEFTBRACKET, '[', '{' }, { SDL_SCANCODE_RIGHTBRACKET, ']', '}' },
		{ SDL_SCANCODE_BACKSLASH, '\\', '|' }, { SDL_SCANCODE_SEMICOLON, ';', ':' },
		{ SDL_SCANCODE_APOSTROPHE, '\'', '"' }, { SDL_SCANCODE_GRAVE, '`', '~' },
		{ SDL_SCANCODE_COMMA, ',', '<' }, { SDL_SCANCODE_PERIOD, '.', '>' },
		{ SDL_SCANCODE_SLASH, '/', '?' },
	};

	for (const auto& e : kShift)
	{
		if (e.sc == sc)
		{
			dest[0] = shift ? e.shifted : e.base;
			dest[1] = 0;
			return true;
		}
	}

	if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
	{
		const bool upper = (shift != caps); // shift XOR caps-lock, same as a real keyboard
		dest[0] = static_cast<char>((upper ? 'A' : 'a') + (sc - SDL_SCANCODE_A));
		dest[1] = 0;
		return true;
	}

	if (sc == SDL_SCANCODE_SPACE)
	{
		dest[0] = ' ';
		dest[1] = 0;
		return true;
	}

	return false;
}

#define MOUSE_1 (0xED + 100)
#define MOUSE_8 (0xED + 107)

BOOL CInput::iGetAsyncKeyState(int dik)
{
	if (dik < COUNT_KB_BUTTONS)
		return !!KBState[dik];
	else if (dik >= MOUSE_1 && dik <= MOUSE_8)
	{
		int mk = dik - MOUSE_1;
		return iGetAsyncBtnState(mk);
	}
	else
		return FALSE; //unknown key ???
}

BOOL CInput::iGetAsyncBtnState(int btn)
{
	// No GetSystemMetrics(SM_SWAPBUTTON)-style userspace swap needed here
	// (see MouseUpdate()'s comment) - X11 already reports a left-handed-
	// configured mouse's buttons pre-swapped.
	return !!mouseState[btn];
}

void CInput::MouseUpdate()
{
#ifndef _EDITOR
	if (Device.dwPrecacheFrame)
		return;
#endif

	static BOOL mouse_prev[COUNT_MOUSE_BUTTONS] = {};
	memcpy(mouse_prev, mouseState, sizeof(mouseState));

	const Uint32 buttons = SDL_GetMouseState(nullptr, nullptr);

	// DirectInput's classic button ordering (0=left,1=right,2=middle,
	// 3=X1,4=X2) vs SDL2's SDL_BUTTON_* numbering (LEFT=1,MIDDLE=2,
	// RIGHT=3,X1=4,X2=5) - remapped once here rather than changing every
	// call site's button-index convention (imgui_helper.h and friends
	// still expect DirectInput ordering). No left-handed-mouse swap logic
	// needed the way the original's GetSystemMetrics(SM_SWAPBUTTON) had -
	// X11 already swaps physical button events for a left-handed-
	// configured mouse below SDL2.
	static const int kSdlButtonToIndex[6] = { -1, 0, 2, 1, 3, 4 }; // indexed by SDL_BUTTON_* (1-based)

	ZeroMemory(mouseState, sizeof(mouseState));
	for (int sdlButton = 1; sdlButton <= 5; ++sdlButton)
		if (buttons & SDL_BUTTON(sdlButton))
			mouseState[kSdlButtonToIndex[sdlButton]] = TRUE;

	for (int i = 0; i < 5; ++i)
	{
		if (mouseState[i] && !mouse_prev[i])
			cbStack.back()->IR_OnMousePress(i);
		else if (!mouseState[i] && mouse_prev[i])
			cbStack.back()->IR_OnMouseRelease(i);
		else if (mouseState[i] && mouse_prev[i])
			cbStack.back()->IR_OnMouseHold(i);
	}

	int dx = 0, dy = 0;
	SDL_GetRelativeMouseState(&dx, &dy);
	offs[0] = dx;
	offs[1] = dy;

	// SDL2's polling API has no wheel-delta equivalent - the wheel only
	// shows up as an SDL_MOUSEWHEEL *event*, so it's peeked out of the
	// event queue here instead of polled (same SDL_PeepEvents approach
	// OpenXRay's own SDL2-based xr_input.cpp uses for the same reason).
	offs[2] = 0;
	{
		SDL_Event wheelEvents[16];
		SDL_PumpEvents();
		const int count = SDL_PeepEvents(wheelEvents, 16, SDL_GETEVENT, SDL_MOUSEWHEEL, SDL_MOUSEWHEEL);
		for (int i = 0; i < count; ++i)
			offs[2] += wheelEvents[i].wheel.y;
	}

	if (offs[0] || offs[1])
	{
		timeStamp[0] = timeStamp[1] = dwCurTime;
		cbStack.back()->IR_OnMouseMove(offs[0], offs[1]);
	}
	if (offs[2])
		cbStack.back()->IR_OnMouseWheel(offs[2]);

	if (!offs[0] && !offs[1])
	{
		if (timeStamp[1] && ((dwCurTime - timeStamp[1]) >= mouse_property.mouse_dt)) cbStack
		                                                                             .back()->IR_OnMouseStop(
			                                                                             DIMOFS_Y, timeStamp[1] = 0);
		if (timeStamp[0] && ((dwCurTime - timeStamp[0]) >= mouse_property.mouse_dt)) cbStack
		                                                                             .back()->IR_OnMouseStop(
			                                                                             DIMOFS_X, timeStamp[0] = 0);
	}
}

//-------------------------------------------------------
void CInput::iCapture(IInputReceiver* p)
{
	VERIFY(p);
	MouseUpdate();
	KeyUpdate();

	// change focus
	if (!cbStack.empty())
		cbStack.back()->IR_OnDeactivate();
	cbStack.push_back(p);
	cbStack.back()->IR_OnActivate();

	// prepare for _new_ controller
	ZeroMemory(timeStamp, sizeof(timeStamp));
	ZeroMemory(timeSave, sizeof(timeStamp));
	ZeroMemory(offs, sizeof(offs));
}

void CInput::iRelease(IInputReceiver* p)
{
	if (p == cbStack.back())
	{
		cbStack.back()->IR_OnDeactivate();
		cbStack.pop_back();
		IInputReceiver* ir = cbStack.back();
		ir->IR_OnActivate();
	}
	else
	{
		// we are not topmost receiver, so remove the nearest one
		u32 cnt = cbStack.size();
		for (; cnt > 0; --cnt)
			if (cbStack[cnt - 1] == p)
			{
				xr_vector<IInputReceiver*>::iterator it = cbStack.begin();
				std::advance(it, cnt - 1);
				cbStack.erase(it);
				break;
			}
	}
}

void CInput::OnAppActivate(void)
{
	if (CurrentIR())
		CurrentIR()->IR_OnActivate();

	SetAllAcquire(true);
	ZeroMemory(mouseState, sizeof(mouseState));
	ZeroMemory(KBState, sizeof(KBState));
	ZeroMemory(timeStamp, sizeof(timeStamp));
	ZeroMemory(timeSave, sizeof(timeStamp));
	ZeroMemory(offs, sizeof(offs));
}

void CInput::OnAppDeactivate(void)
{
	if (CurrentIR())
		CurrentIR()->IR_OnDeactivate();

	SetAllAcquire(false);
	ZeroMemory(mouseState, sizeof(mouseState));
	ZeroMemory(KBState, sizeof(KBState));
	ZeroMemory(timeStamp, sizeof(timeStamp));
	ZeroMemory(timeSave, sizeof(timeStamp));
	ZeroMemory(offs, sizeof(offs));
}

void CInput::DeactivateSoft()
{
	if (CurrentIR())
		CurrentIR()->IR_OnDeactivate();

	ZeroMemory(mouseState, sizeof(mouseState));
	ZeroMemory(KBState, sizeof(KBState));
	ZeroMemory(timeStamp, sizeof(timeStamp));
	ZeroMemory(timeSave, sizeof(timeStamp));
	ZeroMemory(offs, sizeof(offs));
}

void CInput::OnFrame(void)
{
	RDEVICE.Statistic->Input.Begin();
	dwCurTime = RDEVICE.TimerAsync_MMT();
	KeyUpdate();
	MouseUpdate();
	RDEVICE.Statistic->Input.End();
}

IInputReceiver* CInput::CurrentIR()
{
	if (cbStack.size())
		return cbStack.back();
	else
		return NULL;
}

void CInput::unacquire()
{
	SetAllAcquire(FALSE);
}

void CInput::acquire(const bool& exclusive)
{
	(void)exclusive;
	SetAllAcquire(TRUE);
}

void CInput::exclusive_mode(const bool& exclusive)
{
	g_exclusive = exclusive;
	unacquire();
	acquire(exclusive);
}

bool CInput::get_exclusive_mode()
{
	return g_exclusive;
}

void CInput::feedback(u16 s1, u16 s2, float time)
{
	stop_vibration_time = RDEVICE.fTimeGlobal + time;
#ifndef _EDITOR
	//. set_vibration (s1, s2);
#endif
}
