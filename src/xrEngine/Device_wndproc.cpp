#include "stdafx.h"
#include "MonitorList.h"

// Replaces the old Win32 WndProc(hWnd, uMsg, wParam, lParam)/on_message()
// dispatch with a single SDL_Event in - see
// playground/xray-monolith-vulkan-port-notes.md, the SDL2/Vulkan
// window+device bootstrap pass, for the WM_*->SDL_* mapping this was built
// from. Kept as its own file (mirroring the original's device.cpp/
// Device_wndproc.cpp split - the message *pump* lives in device.cpp's
// message_loop(), the message *handling* lives here) rather than folded
// together - OpenXRay's own tree has no same-named file to match shape
// against, and the task said either was fine; this keeps each WM_* case
// below traceable 1:1 back to what it used to be.
//
// Return value keeps on_message()'s old bool contract ("did we fully
// handle this, nothing else needs to see it") even though nothing
// currently branches on it (message_loop() calls this and discards the
// result) - kept for the same reason on_message() had it: a future
// ImGui/editor overlay wanting first refusal on window events.
bool CRenderDevice::ProcessEvent(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_WINDOWEVENT:
	{
		// Only react to events for our own main window here - imgui
		// viewport windows (once ported) get handled by their own logic.
		if (!m_sdlWnd || event.window.windowID != SDL_GetWindowID(m_sdlWnd))
			break;

		switch (event.window.event)
		{
		// WM_ACTIVATE
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_RESTORED:
			OnWindowActivate(true, false);
			return false;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			OnWindowActivate(false, false);
			return false;

		case SDL_WINDOWEVENT_MINIMIZED:
			OnWindowActivate(false, true);
			return false;

		// WM_CLOSE
		case SDL_WINDOWEVENT_CLOSE:
			Engine.Event.Defer("KERNEL:disconnect");
			Engine.Event.Defer("KERNEL:quit");
			return true;

		// WM_DISPLAYCHANGE (the "window moved to a different monitor at a
		// different DPI/refresh rate" half of it - SDL_DISPLAYEVENT below
		// covers monitor hotplug)
		case SDL_WINDOWEVENT_DISPLAY_CHANGED:
			InterlockedExchange(&g_monitor_list_dirty, 1);
			return false;
		} // switch (event.window.event)
		break;
	}

	// WM_DISPLAYCHANGE (monitor hotplug half)
	case SDL_DISPLAYEVENT:
		switch (event.display.type)
		{
		case SDL_DISPLAYEVENT_CONNECTED:
		case SDL_DISPLAYEVENT_DISCONNECTED:
		case SDL_DISPLAYEVENT_ORIENTATION:
			InterlockedExchange(&g_monitor_list_dirty, 1);
			break;
		}
		return false;

	// WM_CHAR. Win32's WM_CHAR delivered one UTF-16 code unit per message;
	// SDL_TEXTINPUT delivers a whole (usually 1-char, but not always)
	// null-terminated UTF-8 string per event. imgui_base.h's InputChar()
	// still has the old single-WPARAM-code-unit shape (imgui_base.cpp
	// itself isn't ported yet, see this batch's CMakeLists.txt exclusion
	// list), so this only forwards the first byte for now - correct for
	// plain ASCII text, not a real UTF-8 decode. Whoever ports
	// imgui_base.cpp should switch this to ImGui's own
	// io.AddInputCharactersUTF8(event.text.text)-style full-string API
	// instead of trying to make InputChar() UTF-8-aware.
	case SDL_TEXTINPUT:
		Device.imgui().InputChar(static_cast<WPARAM>(event.text.text[0]));
		return false;

	// WM_SYSKEYDOWN / WM_SYSCOMMAND(SC_MOVE/SC_SIZE/SC_MAXIMIZE/
	// SC_MONITORPOWER) / WM_SETCURSOR / WM_HOTKEY / WM_SYSCHAR /
	// WM_INPUTLANGCHANGE: every one of these existed to suppress a
	// Win32-only side effect (the "ding" on Alt+key, the OS resetting the
	// cursor icon after every mouse-move, a fullscreen window being
	// dragged/resized/losing power via its system menu) or to observe
	// Win32-only state (the active keyboard layout/IME). None of that
	// exists as a problem to prevent under SDL2/X11 in the first place -
	// this is a case of the underlying Win32 behavior simply not existing
	// on this platform, not an unported gap.
	} // switch (event.type)

	return false;
}
