// Minimal, barebones stand-in for the real PLC_calc3_x86 implementation
// (PLC.cpp, not built - "per-light contribution" calculation, real Win32
// codebase's file predates this port). Its signature takes a real
// CRenderDevice& and light* from the still out-of-scope renderer
// (Layers/xrRender/light.h - not built, see the render stub's own notes
// in Layers/xrAPI/render_stub.cpp) - since nothing renders yet, there is
// no real light data to compute against, so this stays a no-op exactly
// like the rest of the renderer stub, rather than porting PLC.cpp's real
// math against a light class that doesn't exist in this build.
//
// Only forward declarations are needed here (CRenderDevice&/light* are
// reference/pointer parameters - a function that never dereferences them
// doesn't need their complete type), matching xrCPU_Pipe.h's own existing
// forward declarations.

#include "stdafx.h"
#include "xrCPU_Pipe.h"

void __stdcall PLC_calc3_x86(int& c0, int& c1, int& c2, CRenderDevice& /*Device*/, Fvector* /*P*/, Fvector& /*N*/,
                              light* /*L*/, float /*energy*/, Fvector& /*O*/)
{
	c0 = c1 = c2 = 0;
}
