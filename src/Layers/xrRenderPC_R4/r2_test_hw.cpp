#include "stdafx.h"

// On real Windows this probed for DX11 support by dynamically loading
// d3d11.dll, registering a throwaway window class, creating a dummy window,
// and attempting D3D11CreateDeviceAndSwapChain through it - a way to check
// hardware/driver DX11 support before committing to the R4 render path.
// Under dxvk-native, D3D11 is provided directly (linked in, not a system
// DLL to probe), and the real device-creation capability check happens in
// CHW::CreateD3D - so this gate can simply pass through.
BOOL xrRender_test_hw() { return TRUE; }
