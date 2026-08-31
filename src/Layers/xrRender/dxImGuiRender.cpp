#include "stdafx.h"
#include "dxImGuiRender.h"

// Dear ImGui's DX9/10/11 backends (backends/imgui_impl_dx9/10/11.*) are
// deliberately not built anywhere in this port - see the "Explicitly
// excluded" note in sdk/include/imgui/CMakeLists.txt. That comment
// reasoned backend integration was out of scope pre-dxvk since there was
// no working renderer to feed draw data into; now that xrRenderPC_R4 is a
// real DX11 target the reasoning still holds for a stronger reason: the
// DX11 backend (imgui_impl_dx11.cpp) compiles its debug-UI shaders at
// runtime via D3DCompile() (see its own #include <d3dcompiler.h>), and
// dxvk-native is wired into this CMake tree headers-only - its Meson
// build (which would provide a real D3DCompile) isn't hooked up (see this
// target's own CMakeLists.txt comment) - so there is nothing to link
// D3DCompile against. Wiring up a real HLSL-at-runtime compiler is a
// separate, much larger task than the dxvk plumbing this pass is after.
//
// dxImGuiRender can't simply be dropped from the build like a normal
// unused file, though: dxRenderFactory.cpp's RENDER_FACTORY_IMPLEMENT
// macro does `xr_new<dxImGuiRender>()` unconditionally (CreateImGuiRender,
// called from xrEngine's imgui_base.cpp), so the class has to exist and
// link. Every method below is therefore a no-op stub instead - the
// in-game ImGui debug overlay simply doesn't render anything yet, rather
// than the build failing or a real backend needing to be authored here.

void dxImGuiRender::Copy(IImGuiRender& /*_in*/)
{
}

void dxImGuiRender::SetState(ImDrawData* /*data*/)
{
}

void dxImGuiRender::Frame()
{
}

void dxImGuiRender::Render(ImDrawData* /*data*/)
{
}

void dxImGuiRender::OnDeviceCreate(ImGuiContext* /*context*/)
{
}

void dxImGuiRender::OnDeviceDestroy()
{
}

void dxImGuiRender::OnDeviceResetBegin()
{
}

void dxImGuiRender::OnDeviceResetEnd()
{
}
