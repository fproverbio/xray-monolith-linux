#include "stdafx.h"
#include "../xrCDB/Frustum.h"
#include "XR_IOConsole.h"
#include "xr_input.h"
#include "../xrCore/profiler.h"

#include "x_ray.h"
// Discord Game SDK only ships a Windows binary in this tree (sdk/libraries/
// x64/discord_game_sdk.lib, no Linux .so anywhere in the checkout) - see
// x_ray.cpp's file comment for the full reasoning. Guarded out here the
// same way, not deleted.
#ifdef _WIN32
#include "discord\discord.h"
#endif
#include "Render.h"
#include <chrono>

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "../xrCore/FS_impl.h"

#ifdef INGAME_EDITOR
# include "../include/editor/ide.hpp"
# include "engine_impl.hpp"
#endif // #ifdef INGAME_EDITOR

#include "xrSASH.h"

// IGame_Level.h/IGame_Persistent.h are not ported yet - both need real
// object-graph/environment infrastructure (Environment.h, CameraManager.h,
// bone.h, xrCDB/xr_area.h's excluded .cpp) well outside this window/
// device-bootstrap pass's scope, same exclusion already applied to
// Rain.cpp/Environment_misc.cpp/xr_efflensflare.cpp/etc in this batch's
// CMakeLists.txt (see playground/xray-monolith-vulkan-port-notes.md).
// Every use of g_pGameLevel/g_pGamePersistent below is either a bare
// pointer (fine with just a forward declaration - no member access) or
// explicitly deferred/stubbed with a TODO pointing back to this comment.
class IGame_Level;
extern IGame_Level* g_pGameLevel;
class IGame_Persistent;
extern IGame_Persistent* g_pGamePersistent;

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;


ENGINE_API BOOL g_bRendering = FALSE;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;

BOOL psLua_ParallelGC = TRUE;
BOOL psLua_ParallelGC_debug = FALSE;
int psLua_ParallelGC_CallAmount = 25;

#ifdef _WIN32
extern discord::Core* discord_core;
extern bool use_discord;
#endif

#ifdef ECO_RENDER
std::chrono::high_resolution_clock::time_point tlastf = std::chrono::high_resolution_clock::now(), tcurrentf = std::
	                                               chrono::high_resolution_clock::now();
std::chrono::duration<float> time_span;
ENGINE_API float refresh_rate = 0;
#endif // ECO_RENDER

BOOL CRenderDevice::Begin()
{
	PROF_EVENT();

#ifndef DEDICATED_SERVER
	switch (m_pRender->GetDeviceState())
	{
	case IRenderDeviceRender::dsOK:
		break;

	case IRenderDeviceRender::dsLost:
		// If the device was lost, do not render until we get it back
		Sleep(33);
		return FALSE;
		break;

	case IRenderDeviceRender::dsNeedReset:
		// Check if the device is ready to be reset
		Reset();
		break;

	default:
		R_ASSERT(0);
	}

	m_pRender->Begin();

	FPU::m24r();
	g_bRendering = TRUE;
#endif
	return TRUE;
}

void CRenderDevice::Clear()
{
	m_pRender->Clear();
}

extern void CheckPrivilegySlowdown();


void CRenderDevice::End(void)
{
	PROF_EVENT();

#ifndef DEDICATED_SERVER


#ifdef INGAME_EDITOR
    bool load_finished = false;
#endif // #ifdef INGAME_EDITOR
	if (dwPrecacheFrame)
	{
		::Sound->set_master_volume(0.f);
		dwPrecacheFrame--;

		if (!dwPrecacheFrame)
		{
#ifdef INGAME_EDITOR
            load_finished = true;
#endif // #ifdef INGAME_EDITOR

			m_pRender->updateGamma();

			if (precache_light)
			{
				precache_light->set_active(false);
				precache_light.destroy();
			}
			::Sound->set_master_volume(1.f);

			m_pRender->ResourcesDestroyNecessaryTextures();

			Msg("* [x-ray]: Handled Necessary Textures Destruction");
			Memory.mem_compact();
			Msg("* MEMORY USAGE: %lld K", Memory.mem_usage() / 1024);
			Msg("* End of synchronization A[%d] R[%d]", b_is_Active, b_is_Ready);

#ifdef FIND_CHUNK_BENCHMARK_ENABLE
            g_find_chunk_counter.flush();
#endif // FIND_CHUNK_BENCHMARK_ENABLE

			CheckPrivilegySlowdown();

			// TODO: g_pGamePersistent->GameType() needs IGame_Persistent.h,
			// not ported yet (see the class-forward-declaration comment
			// above) - conservatively skip this "pause if the window lost
			// focus during precache" hack entirely rather than guess at
			// its singleplayer-only ("GameType()==1") intent.
#if 0
			if (g_pGamePersistent->GameType() == 1) //haCk
			{
				const bool focused = (SDL_GetWindowFlags(m_sdlWnd) & SDL_WINDOW_INPUT_FOCUS) != 0;
				if (!focused)
					Pause(TRUE, TRUE, TRUE, "application start");
			}
#endif
		}
	}

	g_bRendering = FALSE;
	// end scene
	// Present goes here, so call OA Frame end.
	if (g_SASH.IsBenchmarkRunning())
		g_SASH.DisplayFrame(Device.fTimeGlobal);
	m_pRender->End();

# ifdef INGAME_EDITOR
    if (load_finished && m_editor)
        m_editor->on_load_finished();
# endif // #ifdef INGAME_EDITOR
#endif
}


volatile u32 mt_Thread_marker = 0x12345678;

void mt_Thread(void* ptr)
{
	auto& device = *static_cast<CRenderDevice*>(ptr);
	while (true)
	{
		PROF_EVENT();

		START_PROFILE("Wait for device");
		// waiting for Device permission to execute
		device.mt_csEnter.Enter();

		if (device.mt_bMustExit)
		{
			PROF_EVENT("Must exit");

			device.mt_bMustExit = FALSE; // Important!!!
			device.mt_csEnter.Leave(); // Important!!!
			return;
		}
		// we has granted permission to execute
		mt_Thread_marker = device.dwFrame;
		STOP_PROFILE;

		START_PROFILE("Process seqParallel");
		for (u32 pit = 0; pit < device.seqParallel.size(); pit++)
			device.seqParallel[pit]();
		device.seqParallel.clear_not_free();
		STOP_PROFILE;

		START_PROFILE("Process seqFrameMT");
		device.seqFrameMT.Process(rp_Frame);
		STOP_PROFILE;

		// demonized: While Renderer prepares frame and GPU renders it, use time opportunity to repeatedly call Lua GC with small step value
		// Reduces stutters since less work will be done in main GC step or no work at all
		{
			PROF_EVENT("seqLuaGC");
			if (psLua_ParallelGC && Device.LuaGC)
			{
				// Do at least once
				do
				{
					Device.LuaGCCount++;
					if (Device.LuaGC() == 1) // 1 informs that GC cycle is complete
					{
						Device.LuaGCDone = true;
						break;
					}

				} while (Device.isRendering && Device.LuaGCCount < psLua_ParallelGC_CallAmount);
			}
		}

		START_PROFILE("Synchronization");
		// now we give control to device - signals that we are ended our work
		device.mt_csEnter.Leave();
		// waits for device signal to continue - to start again
		device.mt_csLeave.Enter();
		// returns sync signal to device
		device.mt_csLeave.Leave();
		STOP_PROFILE;
	}
}

void CRenderDevice::PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
#ifdef DEDICATED_SERVER
    amount = 0;
#else
	if (m_pRender->GetForceGPU_REF())
		amount = 0;
#endif

	dwPrecacheFrame = dwPrecacheTotal = amount;
	if (amount && !precache_light && g_pGameLevel && g_loading_events.empty())
	{
		precache_light = ::Render->light_create();
		precache_light->set_shadow(false);
		precache_light->set_position(vCameraPosition);
		precache_light->set_color(255, 255, 255);
		precache_light->set_range(5.0f);
		precache_light->set_active(true);
	}

	if (amount && b_draw_loadscreen && !load_screen_renderer.b_registered)
	{
		load_screen_renderer.start(b_wait_user_input);
	}
}

int g_svDedicateServerUpdateReate = 100;

ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

extern bool IsMainMenuActive(); //ECO_RENDER add

// Monitor query/selection - ported from Win32 EnumDisplayMonitors/
// GetMonitorInfoA/MonitorFromPoint to SDL2's display-query API (see
// MonitorList.cpp's file comment for the full HMONITOR-encoding
// rationale; xr_MonitorFromDisplayIndex()/xr_DisplayIndexFromMonitor()
// live in MonitorList.h so both files share the identical convention).
static HMONITOR g_StartupMonitor = nullptr;

#include "MonitorList.h"

static void InitMonitor()
{
	if (g_StartupMonitor)
		return;

	HMONITOR chosen = ResolveSelectedMonitor();
	if (chosen)
	{
		g_StartupMonitor = chosen;
		return;
	}

	// "Auto": whichever display currently contains the mouse cursor -
	// closest SDL2 equivalent of Win32's
	// MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY). Falls back to
	// display 0 if nothing matches (e.g. no real display server - this
	// port's documented headless-test-environment limitation).
	int x = 0, y = 0;
	SDL_GetGlobalMouseState(&x, &y);

	int display = 0;
	const int numDisplays = SDL_GetNumVideoDisplays();
	for (int i = 0; i < numDisplays; ++i)
	{
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(i, &bounds) == 0 &&
			x >= bounds.x && x < bounds.x + bounds.w &&
			y >= bounds.y && y < bounds.y + bounds.h)
		{
			display = i;
			break;
		}
	}
	g_StartupMonitor = xr_MonitorFromDisplayIndex(display);
}

ENGINE_API void ResetStartupMonitor()
{
	g_StartupMonitor = nullptr;
}

ENGINE_API void SetStartupMonitor(HMONITOR h)
{
	g_StartupMonitor = h;
}

ENGINE_API HMONITOR GetStartupMonitor()
{
	InitMonitor();
	return g_StartupMonitor;
}

void GetMonitorResolution(u32& horizontal, u32& vertical)
{
	InitMonitor();

	SDL_Rect bounds;
	const int display = xr_DisplayIndexFromMonitor(g_StartupMonitor);
	if (display >= 0 && SDL_GetDisplayBounds(display, &bounds) == 0)
	{
		horizontal = static_cast<u32>(bounds.w);
		vertical = static_cast<u32>(bounds.h);
	}
	else
	{
		// No real display server available (see this port's documented
		// headless-test-environment limitation) - a sane desktop default.
		horizontal = 1024;
		vertical = 768;
	}
}

void GetMonitorPosition(int& x, int& y)
{
	InitMonitor();

	SDL_Rect bounds;
	const int display = xr_DisplayIndexFromMonitor(g_StartupMonitor);
	if (display >= 0 && SDL_GetDisplayBounds(display, &bounds) == 0)
	{
		x = bounds.x;
		y = bounds.y;
	}
	else
	{
		x = 0;
		y = 0;
	}
}

float GetMonitorRefresh()
{
	InitMonitor();

	SDL_DisplayMode mode;
	const int display = xr_DisplayIndexFromMonitor(g_StartupMonitor);
	if (display >= 0 && SDL_GetCurrentDisplayMode(display, &mode) == 0 && mode.refresh_rate > 0)
		return 1.f / static_cast<float>(mode.refresh_rate);

	return 1.f / 60.f;
}

extern int ps_framelimiter;
extern u32 g_screenmode;

CTimer FreezeTimer;
void mt_FreezeThread(void *ptr) {
	float freezetime = 0.f;
	float repeatcheck = 500.f;

	while (true)
	{
		PROF_EVENT();

		if (g_loading_events.size())
			freezetime = 25000.0f;
		else
			freezetime = 5000.0f;

		repeatcheck = 500.f;

		START_PROFILE("Check timer");
		if (FreezeTimer.GetElapsed_sec()*1000.f > freezetime)
		{
			FlushLog();
			repeatcheck = 5000.f;
		}
		STOP_PROFILE;

		Sleep(repeatcheck);
	}
}

void CRenderDevice::on_idle()
{
	FreezeTimer.Start();

	if (!b_is_Ready)
	{
		Sleep(100);
		return;
	}

	PROF_FRAME("X-RAY Primary thread");
	PROF_EVENT();

#ifdef DEDICATED_SERVER
    u32 FrameStartTime = TimerGlobal.GetElapsed_ms();
#endif

	START_PROFILE("Set stat gathering");
	if (psDeviceFlags.test(rsStatistic))
		g_bEnableStatGather = TRUE;
	else g_bEnableStatGather = FALSE;
	STOP_PROFILE;

	if (g_loading_events.size())
	{
		PROF_EVENT("Pop loading event");
		if (g_loading_events.front()())
			g_loading_events.pop_front();
		pApp->LoadDraw();
		return;
	}

	if (!Device.dwPrecacheFrame && !g_SASH.IsBenchmarkRunning() && g_bLoaded)
	{
		PROF_EVENT("Start xrSASH Benchmark");
		g_SASH.StartBenchmark();
	}

	FrameMove();

	// Precache
	if (dwPrecacheFrame)
	{
		PROF_EVENT("Precache frame");
		float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
		float angle = PI_MUL_2 * factor;
		vCameraDirection.set(_sin(angle), 0, _cos(angle));
		vCameraDirection.normalize();
		vCameraTop.set(0, 1, 0);
		vCameraRight.crossproduct(vCameraTop, vCameraDirection);

		mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
	}

	// Matrices
	START_PROFILE("Matrices");
	mFullTransform.mul(mProject, mView);
	mFullTransformHud.mul(mProjectHud, mView);
	m_pRender->SetCacheXform(mView, mProject);

	// Previous frame data --
	mView_prev = mView_saved;
	mProject_prev = mProject_saved;
	//mFullTransform_prev = mFullTransform_saved; // Unused?

	m_pRender->SetCacheXform_prev(mView_prev, mProject_prev);

	// TODO: grass-bender/wind-anim data sync below needs
	// IGame_Persistent::grass_shader_data + Environment() (Environment.h,
	// itself excluded from this batch - see the class-forward-declaration
	// comment near the top of this file). Deferred along with those.
#if 0
	// Save previous frame grass benders data
	IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;

	GData.prev_pos[0].set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, -1);
	GData.prev_dir[0].set(0.0f, -99.0f, 0.0f, 1.0f);

	for (int pBend = 1; pBend < _min(16, ps_ssfx_grass_interactive.y + 1); pBend++)
	{
		GData.prev_pos[pBend].set(GData.pos[pBend].x, GData.pos[pBend].y, GData.pos[pBend].z, GData.radius_curr[pBend]);
		GData.prev_dir[pBend].set(GData.dir[pBend].x, GData.dir[pBend].y, GData.dir[pBend].z, GData.str[pBend]);
	}

	// Save wind animation position
	wind_anim_prev = wind_anim_saved;
	wind_anim_saved = g_pGamePersistent->Environment().wind_anim;
#endif

	// Real, general 4x4 inverse (replaces D3DXMatrixInverse, which has no
	// portable equivalent - see xrCore/_matrix.h's invert_44() for the
	// full rationale; mFullTransform = mProject*mView is a genuine
	// projective matrix, not just rotation+translation, so the existing
	// invert()/invert_b() 4x3-only methods would silently give wrong
	// results here).
	mInvFullTransform.invert_44(mFullTransform);

	vCameraPosition_saved = vCameraPosition;
	mFullTransform_saved = mFullTransform;
	mView_saved = mView;
	mProject_saved = mProject;
	STOP_PROFILE;

	Device.isRendering = true;
	Device.LuaGCCount = 0;
	Device.LuaGCDone = false;

	// *** Resume threads
	// Capture end point - thread must run only ONE cycle
	// Release start point - allow thread to run
	START_PROFILE("Resume threads");
	mt_csLeave.Enter();
	mt_csEnter.Leave();
	STOP_PROFILE;

#ifdef ECO_RENDER // ECO_RENDER START
	if (Device.Paused() || IsMainMenuActive() || ps_framelimiter)
	{
		PROF_EVENT("Eco Render");

		if (refresh_rate == 0)
			refresh_rate = GetMonitorRefresh();

		float rr;

		if (ps_framelimiter)
			rr = 1.f / ps_framelimiter;
		else
			rr = refresh_rate;

		time_span = std::chrono::duration_cast<std::chrono::duration<float>>(tcurrentf - tlastf);
		while (time_span.count() < rr)
		{
			tcurrentf = std::chrono::high_resolution_clock::now();
			time_span = std::chrono::duration_cast<std::chrono::duration<float>>(tcurrentf - tlastf);
		}
		tlastf = std::chrono::high_resolution_clock::now();
	}
#endif // ECO_RENDER END

#ifndef DEDICATED_SERVER
	Statistic->RenderTOTAL_Real.FrameStart();
	Statistic->RenderTOTAL_Real.Begin();

	if (b_is_Active && Begin())
	{
		START_PROFILE("Process seqRender");
		seqRender.Process(rp_Render);
		STOP_PROFILE;

		if (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Statistic->errors.size())
		{
			PROF_EVENT("Draw statistics");
			Statistic->Show();
		}

		End();
	}
	Statistic->RenderTOTAL_Real.End();
	Statistic->RenderTOTAL_Real.FrameEnd();
	Statistic->RenderTOTAL.accum = Statistic->RenderTOTAL_Real.accum;
#endif // #ifndef DEDICATED_SERVER
	Device.isRendering = false;

	// *** Suspend threads
	// Capture startup point
	// Release end point - allow thread to wait for startup point
	START_PROFILE("Suspend threads");
	mt_csEnter.Enter();
	mt_csLeave.Leave();
	STOP_PROFILE;

	// Ensure, that second thread gets chance to execute anyway
	if (dwFrame != mt_Thread_marker)
	{
		PROF_EVENT("Execute second thread");
		for (u32 pit = 0; pit < Device.seqParallel.size(); pit++)
			Device.seqParallel[pit]();
		Device.seqParallel.clear_not_free();
		seqFrameMT.Process(rp_Frame);
	}

	if (psLua_ParallelGC_debug && psLua_ParallelGC && Device.LuaGCDebug)
	{
		Device.LuaGCDebug();
	}

#ifdef DEDICATED_SERVER
    u32 FrameEndTime = TimerGlobal.GetElapsed_ms();
    u32 FrameTime = (FrameEndTime - FrameStartTime);
    u32 DSUpdateDelta = 1000 / g_svDedicateServerUpdateReate;
    if (FrameTime < DSUpdateDelta)
        Sleep(DSUpdateDelta - FrameTime);
#endif
	if (!b_is_Active)
		Sleep(1);
}

#ifdef INGAME_EDITOR
void CRenderDevice::message_loop_editor()
{
    m_editor->run();
    m_editor_finalize(m_editor);
    xr_delete(m_engine);
}
#endif // #ifdef INGAME_EDITOR

void CRenderDevice::Screenshot()
{
	PROF_EVENT();
	Render->Screenshot();
}

void CRenderDevice::message_loop()
{
#ifdef INGAME_EDITOR
    if (editor())
    {
        message_loop_editor();
        return;
    }
#endif
	// Replaces the old Win32 PeekMessage/TranslateMessage/DispatchMessage
	// pump: drain every pending SDL event through ProcessEvent()
	// (Device_wndproc.cpp), then run one on_idle() - same overall shape
	// (dispatch pending input, then idle-render), except every pending
	// event is drained per iteration instead of one message at a time,
	// which is both simpler and avoids event-floods starving on_idle().
	SDL_Event event;
	for (;;)
	{
		bool quit = false;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				quit = true;
				continue;
			}
			ProcessEvent(event);
		}
		if (quit)
			break;
		on_idle();
	}
}

#ifdef _WIN32
void mt_DiscordThread(void*)
{
	while (true)
	{
		if (!pApp)
		{
			Msg("[Discord] pApp destroyed, killing thread");
			return;
		}

		//Discord
		if (use_discord && psDeviceFlags2.test(rsDiscord))
		{
			START_PROFILE("Discord");
			discord_core->RunCallbacks();
			updateDiscordPresence();
			STOP_PROFILE;
			Sleep(int(discord_update_rate * 1000));
		}
		else
		{
			Sleep(1000); // Sleep for 1 second if Discord is not used or disabled
		}
	}
}
#endif // #ifdef _WIN32

void CRenderDevice::Run()
{
	// DUMP_PHASE;
	g_bLoaded = FALSE;
	Log("Starting engine...");
	thread_name("X-RAY Primary thread");
	// Startup timers and calculate timer delta
	dwTimeGlobal = 0;
	Timer_MM_Delta = 0;
	{
		u32 time_mm = timeGetTime();
		while (timeGetTime() == time_mm); // wait for next tick
		u32 time_system = timeGetTime();
		u32 time_local = TimerAsync();
		Timer_MM_Delta = time_system - time_local;
	}
	// Start all threads
	// InitializeCriticalSection (&mt_csEnter);
	// InitializeCriticalSection (&mt_csLeave);
	mt_csEnter.Enter();
	mt_bMustExit = FALSE;
	thread_spawn(mt_FreezeThread, "Freeze detecting thread", 0, 0);
	thread_spawn(mt_Thread, "X-RAY Secondary thread", 0, this);
#ifdef _WIN32
	thread_spawn(mt_DiscordThread, "X-RAY Discord thread", 0, 0);
#endif
	// Message cycle
	seqAppStart.Process(rp_AppStart);
	m_pRender->ClearTarget();
	// Real window, created hidden in Device_Initialize.cpp - show + raise
	// it now (replaces SetForegroundWindow(m_hWnd)).
	SDL_ShowWindow(m_sdlWnd);
	SDL_RaiseWindow(m_sdlWnd);
	message_loop();
	seqAppEnd.Process(rp_AppEnd);
	// Stop Balance-Thread
	mt_bMustExit = TRUE;
	mt_csEnter.Leave();
	while (mt_bMustExit) Sleep(0);
	// DeleteCriticalSection (&mt_csEnter);
	// DeleteCriticalSection (&mt_csLeave);
}

u32 app_inactive_time = 0;
u32 app_inactive_time_start = 0;

void CRenderDevice::FrameMove()
{
	PROF_EVENT();

	if (InterlockedExchange(&g_monitor_list_dirty, 0))
		refresh_vid_monitor_list();

	dwFrame++;
	Core.dwFrame = dwFrame;
	dwTimeContinual = TimerMM.GetElapsed_ms() - app_inactive_time;
	if (psDeviceFlags.test(rsConstantFPS))
	{
		PROF_EVENT("Constant FPS");

		// 20ms = 50fps
		//fTimeDelta = 0.020f;
		//fTimeGlobal += 0.020f;
		//dwTimeDelta = 20;
		//dwTimeGlobal += 20;
		// 33ms = 30fps
		fTimeDelta = 0.033f;
		fTimeGlobal += 0.033f;
		dwTimeDelta = 33;
		dwTimeGlobal += 33;
	}
	else
	{
		PROF_EVENT("Timer FPS");

		// Timer
		float fPreviousFrameTime = Timer.GetElapsed_sec();
		Timer.Start(); // previous frame
		fTimeDelta = 0.1f * fTimeDelta + 0.9f * fPreviousFrameTime;
		// smooth random system activity - worst case ~7% error
		//fTimeDelta = 0.7f * fTimeDelta + 0.3f*fPreviousFrameTime; // smooth random system activity
		if (fTimeDelta > .1f)
			fTimeDelta = .1f; // limit to 15fps minimum
		if (fTimeDelta <= 0.f)
			fTimeDelta = EPS_S + EPS_S; // limit to 15fps minimum
		if (Paused())
			fTimeDelta = 0.0f;
		// u64 qTime = TimerGlobal.GetElapsed_clk();
		fTimeGlobal = TimerGlobal.GetElapsed_sec(); //float(qTime)*CPU::cycles2seconds;
		u32 _old_global = dwTimeGlobal;
		dwTimeGlobal = TimerGlobal.GetElapsed_ms();
		dwTimeDelta = dwTimeGlobal - _old_global;
	}
	// Frame move
	Statistic->EngineTOTAL.Begin();
	// TODO: HACK to test loading screen.
	//if(!g_bLoaded)
	START_PROFILE("Process seqFrame");
	Device.seqFrame.Process(rp_Frame);
	STOP_PROFILE;
	g_bLoaded = TRUE;
	//else
	// seqFrame.Process(rp_Frame);
	Statistic->EngineTOTAL.End();
}

ENGINE_API BOOL bShowPauseString = TRUE;

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	PROF_EVENT();

	static int snd_emitters_ = -1;

	if (g_bBenchmark)
		return;
#ifndef DEDICATED_SERVER
	if (bOn)
	{
		if (!Paused())
			bShowPauseString =
#ifdef INGAME_EDITOR
                editor() ? FALSE :
#endif // #ifdef INGAME_EDITOR
#ifdef DEBUG
                !xr_strcmp(reason, "li_pause_key_no_clip") ? FALSE :
#endif // DEBUG
				TRUE;

		// TODO: g_pGamePersistent->CanBePaused() needs IGame_Persistent.h,
		// not ported yet (see the class-forward-declaration comment near
		// the top of this file) - pausing the timer unconditionally here
		// is the safe default until that's ported (matches this
		// function's own "if bTimer" gate, just without the extra
		// game-specific opt-out).
		if (bTimer)
		{
			g_pauseMngr().Pause(true);
#ifdef DEBUG
            if (!xr_strcmp(reason, "li_pause_key_no_clip"))
                TimerGlobal.Pause(FALSE);
#endif // DEBUG
		}

		if (bSound && ::Sound)
		{
			snd_emitters_ = ::Sound->pause_emitters(true);
#ifdef DEBUG
			// Log("snd_emitters_[true]",snd_emitters_);
#endif // DEBUG
		}
	}
	else
	{
		if (bTimer && g_pauseMngr().Paused())
		{
			fTimeDelta = EPS_S + EPS_S;
			g_pauseMngr().Pause(false);
		}

		if (bSound)
		{
			if (snd_emitters_ > 0) //avoid crash
			{
				snd_emitters_ = ::Sound->pause_emitters(false);
#ifdef DEBUG
				// Log("snd_emitters_[false]",snd_emitters_);
#endif
			}
			else
			{
#ifdef DEBUG
                Log("Sound->pause_emitters underflow");
#endif
			}
		}
	}

#endif
}

bool CRenderDevice::Paused()
{
	return g_pauseMngr().Paused();
}

void CRenderDevice::OnWindowActivate(bool activated, bool minimized)
{
	BOOL bActive = (activated && !minimized) ? TRUE : FALSE;

	if (psDeviceFlags2.test(rsAlwaysActive) && g_screenmode != 2)
	{
		Device.b_is_Active = TRUE;

		if (Device.b_hide_cursor != bActive)
		{
			Device.b_hide_cursor = bActive;

			if (Device.b_hide_cursor)
			{
				SDL_ShowCursor(SDL_DISABLE);
				// Confine the cursor to the window - closest SDL2
				// equivalent of GetClientRect+MapWindowPoints+ClipCursor
				// (SDL2 has no arbitrary-rect cursor clip, only a
				// whole-window grab).
				if (m_sdlWnd)
					SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);
				pInput->OnAppActivate();
			}
			else
			{
				SDL_ShowCursor(SDL_ENABLE);
				if (m_sdlWnd)
					SDL_SetWindowGrab(m_sdlWnd, SDL_FALSE);
				pInput->OnAppDeactivate();
			}
		}

		return;
	}

	if (bActive != Device.b_is_Active)
	{
		Device.b_is_Active = bActive;

		if (Device.b_is_Active)
		{
			Device.seqAppActivate.Process(rp_AppActivate);
			app_inactive_time += TimerMM.GetElapsed_ms() - app_inactive_time_start;

#ifndef DEDICATED_SERVER
			SDL_ShowCursor(SDL_DISABLE);
			if (m_sdlWnd)
				SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);
#endif // #ifndef DEDICATED_SERVER
		}
		else
		{
			app_inactive_time_start = TimerMM.GetElapsed_ms();
			Device.seqAppDeactivate.Process(rp_AppDeactivate);
			SDL_ShowCursor(SDL_ENABLE);
			if (m_sdlWnd)
				SDL_SetWindowGrab(m_sdlWnd, SDL_FALSE);
		}
	}
}

void CRenderDevice::AddSeqFrame(pureFrame* f, bool mt)
{
	PROF_EVENT();

	if (mt)
		seqFrameMT.Add(f, REG_PRIORITY_HIGH);
	else
		seqFrame.Add(f, REG_PRIORITY_LOW);
}

void CRenderDevice::RemoveSeqFrame(pureFrame* f)
{
	PROF_EVENT();

	seqFrameMT.Remove(f);
	seqFrame.Remove(f);
}

CLoadScreenRenderer::CLoadScreenRenderer()
	: b_registered(false)
{
}

void CLoadScreenRenderer::start(bool b_user_input)
{
	PROF_EVENT();

	Device.seqRender.Add(this, 0);
	b_registered = true;
	b_need_user_input = b_user_input;
}

void CLoadScreenRenderer::stop()
{
	PROF_EVENT();

	if (!b_registered)
		return;
	Device.seqRender.Remove(this);
	pApp->destroy_loading_shaders();
	b_registered = false;
	b_need_user_input = false;
}

void CLoadScreenRenderer::OnRender()
{
	PROF_EVENT();

	pApp->load_draw_internal();
}

void CRenderDevice::CSecondVPParams::SetSVPActive(bool bState) //--#SM+#-- +SecondVP+
{
	isActive = bState;
	// TODO: g_pGamePersistent->m_pGShaderConstants sync needs
	// IGame_Persistent.h, not ported yet (see the class-forward-
	// declaration comment near the top of this file) - deferred.
}

bool CRenderDevice::CSecondVPParams::IsSVPFrame() //--#SM+#-- +SecondVP+
{
	return IsSVPActive() && Device.dwFrame % frameDelay == 0;
}
