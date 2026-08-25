////////////////////////////////////////////////////////////////////////////
//	Module 		: script_render_device_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script render device script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_render_device.h"

using namespace luabind;

bool is_device_paused(CRenderDevice* d)
{
	return !!Device.Paused();
}

void set_device_paused(CRenderDevice* d, bool b)
{
	Device.Pause(b, TRUE, FALSE, "set_device_paused_script");
}

extern ENGINE_API BOOL bShowPauseString;

void set_device_paused_ex(CRenderDevice* d, bool b)
{
	Device.Pause(b, TRUE, TRUE, "set_device_paused_ex_script");
	bShowPauseString = FALSE;
}

extern ENGINE_API BOOL g_appLoaded;

bool is_app_ready()
{
	return !!g_appLoaded;
}

u32 time_global(const CRenderDevice* self)
{
	THROW(self);
	return (self->dwTimeGlobal);
}

u32 time_continual(const CRenderDevice* self)
{
	THROW(self);
	return (self->dwTimeContinual);
}

#pragma optimize("s",on)
// CScriptRenderDevice is a typedef for class_exporter<CRenderDevice>
// (script_export_space.h's template). Defining one of its members out of
// line for this one specific T is an explicit specialization, which the
// standard requires a leading `template<>` for - MSVC accepts this without
// it (permissively auto-specializing through the typedef), GCC doesn't.
// Same "MSVC permissive, GCC standard-strict" first-instantiation template
// bug class already hit repeatedly in this port (notes section 16/17a/17d/
// 21c/26b/27d) - likely recurs in many more not-yet-ported *_script.cpp
// files that follow this same class_exporter<T>::script_register pattern.
template<>
void CScriptRenderDevice::script_register(lua_State* L)
{
	module(L)
	[
		class_<CRenderDevice>("render_device")
		.def_readonly("width", &CRenderDevice::dwWidth)
		.def_readonly("height", &CRenderDevice::dwHeight)
		.def_readonly("time_delta", &CRenderDevice::dwTimeDelta)
		.def_readonly("f_time_delta", &CRenderDevice::fTimeDelta)
		.def_readonly("cam_pos", &CRenderDevice::vCameraPosition)
		.def_readonly("cam_dir", &CRenderDevice::vCameraDirection)
		.def_readonly("cam_top", &CRenderDevice::vCameraTop)
		.def_readonly("cam_right", &CRenderDevice::vCameraRight)
		//			.def_readonly("view",					&CRenderDevice::mView)
		//			.def_readonly("projection",				&CRenderDevice::mProject)
		//			.def_readonly("full_transform",			&CRenderDevice::mFullTransform)
		.def_readonly("fov", &CRenderDevice::fFOV)
		.def_readonly("aspect_ratio", &CRenderDevice::fASPECT)
		.def("time_global", &time_global)
		.def("time_continual", &time_continual)
		.def_readonly("precache_frame", &CRenderDevice::dwPrecacheFrame)
		.def_readonly("frame", &CRenderDevice::dwFrame)
		.def("is_paused", &is_device_paused)
		.def("pause", &set_device_paused)
		.def("pause_ex", &set_device_paused_ex),
		def("app_ready", &is_app_ready)
	];
}
