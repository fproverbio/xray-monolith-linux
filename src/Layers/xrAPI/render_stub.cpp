// Minimal, barebones stand-in for the real DX11 renderer
// (Layers/xrRenderPC_R4, ~115 files, not built on this port - a real
// Vulkan backend is deliberately out of scope here, see
// playground/xray-monolith-vulkan-port-notes.md). This file's only job is
// to make the executable *link* by supplying definitions for the handful
// of symbols EngineAPI.cpp calls directly under STATIC_RENDERER_R4
// (DllMainXrRenderR4, SupportsDX11Rendering) plus every dxRenderFactory
// method declared in ../xrRender/dxRenderFactory.h - none of it does real
// work. xrAPI.cpp (same directory) already supplies the Render/
// RenderFactory/UIRender/DU/DRender/PGMLib globals themselves, all left
// NULL; DllMainXrRenderR4 stays a no-op rather than pointing any of them
// at a real object, so nothing here pretends to render anything.

#include "stdafx.h"
#include "../xrRender/dxRenderFactory.h"

dxRenderFactory RenderFactoryImpl;

#define STUB_CREATE(Class) \
	I##Class* dxRenderFactory::Create##Class() { return NULL; }
#define STUB_DESTROY(Class) \
	void dxRenderFactory::Destroy##Class(I##Class* /*pObject*/) {}
#define STUB_FACTORY_METHODS(Class) \
	STUB_CREATE(Class) \
	STUB_DESTROY(Class)

#ifndef _EDITOR
STUB_FACTORY_METHODS(UISequenceVideoItem)
STUB_FACTORY_METHODS(UIShader)
STUB_FACTORY_METHODS(StatGraphRender)
STUB_FACTORY_METHODS(ConsoleRender)
STUB_FACTORY_METHODS(RenderDeviceRender)
#	ifdef DEBUG
STUB_FACTORY_METHODS(ObjectSpaceRender)
#	endif // DEBUG
STUB_FACTORY_METHODS(ApplicationRender)
STUB_FACTORY_METHODS(WallMarkArray)
STUB_FACTORY_METHODS(StatsRender)
#endif // _EDITOR

#ifndef _EDITOR
STUB_FACTORY_METHODS(FlareRender)
STUB_FACTORY_METHODS(ThunderboltRender)
STUB_FACTORY_METHODS(ThunderboltDescRender)
STUB_FACTORY_METHODS(RainRender)
STUB_FACTORY_METHODS(LensFlareRender)
STUB_FACTORY_METHODS(ImGuiRender)
STUB_FACTORY_METHODS(EnvironmentRender)
STUB_FACTORY_METHODS(EnvDescriptorMixerRender)
STUB_FACTORY_METHODS(EnvDescriptorRender)
#endif
STUB_FACTORY_METHODS(FontRender)

#undef STUB_FACTORY_METHODS
#undef STUB_CREATE
#undef STUB_DESTROY

// Real xrRender_R4.cpp's DllMainXrRenderR4 points ::Render/::UIRender/
// ::DU/::DRender at real singleton objects (RImplementation/UIRenderImpl/
// DUImpl/DebugRenderImpl) that don't exist in this build - left NULL
// (xrAPI.cpp's initializers) since there is nothing real to point them
// at yet. ::RenderFactory is already wired to RenderFactoryImpl above via
// xrAPI.cpp's own initializer, so there is genuinely nothing left to do
// here beyond acknowledging the call succeeded.
BOOL DllMainXrRenderR4(HANDLE /*hModule*/, DWORD /*ul_reason_for_call*/, LPVOID /*lpReserved*/)
{
	return TRUE;
}

// No real D3D11/DXGI device to probe - honestly report "no", matching
// this stub's non-goal of rendering anything.
extern "C" bool SupportsDX11Rendering();
bool SupportsDX11Rendering()
{
	return false;
}
