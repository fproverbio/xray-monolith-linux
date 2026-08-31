#pragma once

// dxvk's include/native/directx/d3d11shader.h (a nested submodule,
// misyltoad/mingw-directx-headers) never gives ID3D11ShaderReflection a
// __CRT_UUID_DECL, so __uuidof(ID3D11ShaderReflection)/IID_PPV_ARGS
// against it (used by D3DReflect() call sites in r3.cpp/r4.cpp) fail to
// link with an undefined __uuidof_helper<T>() reference.
//
// __CRT_UUID_DECL only needs the type already declared at the point of
// expansion, not co-located with its own declaration, so this is added
// here instead of patching the vendored header: include after
// <d3dcompiler.h> (which pulls in d3d11shader.h) in stdafx.h.

#include <d3dcompiler.h>

#if D3D_COMPILER_VERSION <= 42
__CRT_UUID_DECL(ID3D11ShaderReflection, 0x17f27486, 0xa342, 0x4d10, 0x88, 0x42, 0xab, 0x08, 0x74, 0xe7, 0xf6, 0x70);
#elif D3D_COMPILER_VERSION == 43
__CRT_UUID_DECL(ID3D11ShaderReflection, 0x0a233719, 0x3960, 0x4578, 0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1);
#else
__CRT_UUID_DECL(ID3D11ShaderReflection, 0x8d536ca1, 0x0cca, 0x4956, 0xa8, 0x37, 0x78, 0x69, 0x63, 0x75, 0x55, 0x84);
#endif
