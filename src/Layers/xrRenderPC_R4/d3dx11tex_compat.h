// Minimal replacement for the proprietary D3DX11 texture-utility library
// (historically shipped as part of the D3DX10/D3DX11 SDK, declared in
// D3DX10Tex.h/D3DX11tex.h), which dxvk-native does not implement and for
// which there is no Linux runtime to link against.
//
// Only the subset actually exercised by this port's DX11 renderer target is
// implemented here: DDS metadata/texture loading from memory
// (D3DX11GetImageInfoFromMemory / D3DX11CreateTextureFromMemory, used by
// dx10Texture.cpp's CRender::texture_load - the game's real texture-loading
// path) and DDS-only texture saving (D3DX11SaveTextureToFile, used by
// dx10Texture.cpp's TW_Save debug helper). Everything is implemented on top
// of the DirectXTex library, which this port already vendors and links
// PUBLIC into xrRenderPC_R4 specifically for this purpose (see
// xrRenderPC_R4/CMakeLists.txt and "3rd party/DirectXTex/CMakeLists.txt"'s
// XR_DIRECTXTEX_DXVK_NATIVE handling).
//
// WIC-backed formats (JPG/PNG/BMP/...) are NOT implemented - DirectXTex's
// WIC codec path is only compiled on _WIN32 (see DirectXTex.h), and there is
// no WIC-equivalent available in this Linux build. D3DX11SaveTextureToFile
// only supports D3DX11_IFF_DDS here; any other requested format fails with
// E_NOTIMPL (its only caller, dx10Texture.cpp's TW_Save debug helper, is
// non-critical). D3DX11SaveTextureToMemory instead always encodes DDS
// regardless of the requested DestFormat - unlike SaveToFile, its callers
// (r__screenshot.cpp) wrap it in CHK_DX(), which is a no-op in release
// builds, so an E_NOTIMPL here would crash on a null blob the moment a
// player presses the screenshot hotkey. Silently substituting DDS keeps
// screenshots working (just with DDS bytes under a .jpg/.png/.tga name)
// instead of crashing; a one-time Msg() documents the substitution.
#ifndef D3DX11TEX_COMPAT_H
#define D3DX11TEX_COMPAT_H
#pragma once

#ifndef USE_DX11
#error "d3dx11tex_compat.h implements the D3DX11 (not D3DX10) compat surface"
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <vector>

#include <DirectXTex.h>

#ifndef D3DX_DEFAULT
#define D3DX_DEFAULT ((UINT)-1)
#endif

//-----------------------------------------------------------------------------
// D3DX11_IMAGE_FILE_FORMAT / D3DX11_IMAGE_INFO / D3DX11_IMAGE_LOAD_INFO
//-----------------------------------------------------------------------------
enum D3DX11_IMAGE_FILE_FORMAT
{
	D3DX11_IFF_BMP = 0,
	D3DX11_IFF_JPG = 1,
	D3DX11_IFF_PNG = 3,
	D3DX11_IFF_DDS = 4,
	D3DX11_IFF_TIFF = 10,
	D3DX11_IFF_GIF = 11,
	D3DX11_IFF_WMP = 12,
};

// Field layout matches the real D3DX11_IMAGE_INFO; callers in this codebase
// ZeroMemory() it themselves before use.
struct D3DX11_IMAGE_INFO
{
	UINT Width;
	UINT Height;
	UINT Depth;
	UINT ArraySize;
	UINT MipLevels;
	UINT MiscFlags;
	D3DX11_IMAGE_FILE_FORMAT ImageFileFormat;
	DXGI_FORMAT Format;
	D3D11_RESOURCE_DIMENSION ResourceDimension;
};

// Field layout matches the real D3DX11_IMAGE_LOAD_INFO. Call sites in this
// codebase rely on the default constructor (as real D3DX11 does) to fill in
// every field they don't explicitly set.
struct D3DX11_IMAGE_LOAD_INFO
{
	UINT Width;
	UINT Height;
	UINT Depth;
	UINT FirstMipLevel;
	UINT MipLevels;
	D3D_USAGE Usage;
	UINT BindFlags;
	UINT CpuAccessFlags;
	UINT MiscFlags;
	DXGI_FORMAT Format;
	UINT Filter;
	UINT MipFilter;
	D3DX11_IMAGE_INFO* pSrcInfo;

	D3DX11_IMAGE_LOAD_INFO() noexcept
	    : Width(D3DX_DEFAULT), Height(D3DX_DEFAULT), Depth(D3DX_DEFAULT), FirstMipLevel(0), MipLevels(D3DX_DEFAULT),
	      Usage(D3D_USAGE_DEFAULT), BindFlags(D3D_BIND_SHADER_RESOURCE), CpuAccessFlags(0), MiscFlags(0),
	      Format(DXGI_FORMAT_UNKNOWN), Filter(D3DX_DEFAULT), MipFilter(D3DX_DEFAULT), pSrcInfo(nullptr)
	{
	}
};

//-----------------------------------------------------------------------------
// D3DX11GetImageInfoFromMemory
//-----------------------------------------------------------------------------
inline HRESULT D3DX11GetImageInfoFromMemory(LPCVOID pSrcData, SIZE_T SrcDataSize, void* /*pPump*/,
                                             D3DX11_IMAGE_INFO* pSrcInfo, HRESULT* pHResult)
{
	DirectX::TexMetadata meta;
	HRESULT hr = DirectX::GetMetadataFromDDSMemory(reinterpret_cast<const uint8_t*>(pSrcData), SrcDataSize,
	                                                DirectX::DDS_FLAGS_NONE, meta);
	if (pHResult)
		*pHResult = hr;
	if (FAILED(hr))
		return hr;

	if (pSrcInfo)
	{
		pSrcInfo->Width = static_cast<UINT>(meta.width);
		pSrcInfo->Height = static_cast<UINT>(meta.height);
		pSrcInfo->Depth = static_cast<UINT>(meta.depth);
		pSrcInfo->ArraySize = static_cast<UINT>(meta.arraySize);
		pSrcInfo->MipLevels = static_cast<UINT>(meta.mipLevels);
		pSrcInfo->MiscFlags = meta.miscFlags;
		pSrcInfo->ImageFileFormat = D3DX11_IFF_DDS;
		pSrcInfo->Format = meta.format;
		pSrcInfo->ResourceDimension = static_cast<D3D11_RESOURCE_DIMENSION>(meta.dimension);
	}
	return S_OK;
}

//-----------------------------------------------------------------------------
// D3DX11CreateTextureFromMemory
//-----------------------------------------------------------------------------
inline HRESULT D3DX11CreateTextureFromMemory(ID3D11Device* pDevice, LPCVOID pSrcData, SIZE_T SrcDataSize,
                                              D3DX11_IMAGE_LOAD_INFO* pLoadInfo, void* /*pPump*/,
                                              ID3D11Resource** ppTexture, HRESULT* pHResult)
{
	DirectX::TexMetadata meta;
	DirectX::ScratchImage image;
	HRESULT hr = DirectX::LoadFromDDSMemory(reinterpret_cast<const uint8_t*>(pSrcData), SrcDataSize,
	                                         DirectX::DDS_FLAGS_NONE, &meta, image);
	if (SUCCEEDED(hr) && meta.depth > 1)
	{
		// Volume (3D) textures aren't loaded through this path anywhere in
		// this codebase - only 2D and cubemap DDS textures are.
		Msg("! D3DX11CreateTextureFromMemory: volume textures are not supported");
		hr = E_NOTIMPL;
	}
	if (pHResult)
		*pHResult = hr;
	if (FAILED(hr))
		return hr;

	UINT firstMip = pLoadInfo && pLoadInfo->FirstMipLevel != D3DX_DEFAULT ? pLoadInfo->FirstMipLevel : 0;
	if (firstMip >= meta.mipLevels)
		firstMip = static_cast<UINT>(meta.mipLevels) - 1;
	UINT available = static_cast<UINT>(meta.mipLevels) - firstMip;
	UINT mipCount = pLoadInfo && pLoadInfo->MipLevels != D3DX_DEFAULT
	                    ? (pLoadInfo->MipLevels < available ? pLoadInfo->MipLevels : available)
	                    : available;
	if (mipCount == 0)
		mipCount = 1;

	std::vector<DirectX::Image> selected;
	selected.reserve(meta.arraySize * mipCount);
	for (size_t item = 0; item < meta.arraySize; ++item)
		for (UINT mip = 0; mip < mipCount; ++mip)
		{
			const DirectX::Image* src = image.GetImage(firstMip + mip, item, 0);
			if (!src)
				return E_FAIL;
			selected.push_back(*src);
		}

	DirectX::TexMetadata subMeta = meta;
	subMeta.mipLevels = mipCount;
	subMeta.width = selected[0].width;
	subMeta.height = selected[0].height;

	D3D_USAGE usage = pLoadInfo && pLoadInfo->Usage != static_cast<D3D_USAGE>(D3DX_DEFAULT) ? pLoadInfo->Usage
	                                                                                         : D3D_USAGE_DEFAULT;
	UINT bindFlags = pLoadInfo && pLoadInfo->BindFlags != D3DX_DEFAULT ? pLoadInfo->BindFlags : D3D_BIND_SHADER_RESOURCE;
	UINT cpuAccess = pLoadInfo && pLoadInfo->CpuAccessFlags != D3DX_DEFAULT ? pLoadInfo->CpuAccessFlags : 0;
	UINT miscFlags = pLoadInfo && pLoadInfo->MiscFlags != D3DX_DEFAULT ? pLoadInfo->MiscFlags : 0;

	hr = DirectX::CreateTextureEx(pDevice, selected.data(), selected.size(), subMeta, usage, bindFlags, cpuAccess,
	                               miscFlags, DirectX::CREATETEX_DEFAULT, ppTexture);
	if (pHResult)
		*pHResult = hr;
	return hr;
}

//-----------------------------------------------------------------------------
// D3DX11SaveTextureToFile (DDS only - see file header comment)
//-----------------------------------------------------------------------------
inline HRESULT D3DX11SaveTextureToFile(ID3D11DeviceContext* pContext, ID3D11Resource* pSrcTexture,
                                        D3DX11_IMAGE_FILE_FORMAT DestFormat, LPCSTR pDestFile)
{
	if (DestFormat != D3DX11_IFF_DDS)
	{
		Msg("! D3DX11SaveTextureToFile: only DDS output is supported on this platform (no WIC)");
		return E_NOTIMPL;
	}

	ID3D11Device* device = nullptr;
	pContext->GetDevice(&device);
	if (!device)
		return E_FAIL;

	DirectX::ScratchImage image;
	HRESULT hr = DirectX::CaptureTexture(device, pContext, pSrcTexture, image);
	device->Release();
	if (FAILED(hr))
		return hr;

	wchar_t wpath[1024];
	std::mbstowcs(wpath, pDestFile, sizeof(wpath) / sizeof(wpath[0]) - 1);
	wpath[sizeof(wpath) / sizeof(wpath[0]) - 1] = L'\0';

	const DirectX::TexMetadata& meta = image.GetMetadata();
	if (meta.mipLevels == 1 && meta.arraySize == 1)
		return DirectX::SaveToDDSFile(*image.GetImage(0, 0, 0), DirectX::DDS_FLAGS_NONE, wpath);

	return DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), meta, DirectX::DDS_FLAGS_NONE, wpath);
}

//-----------------------------------------------------------------------------
// ID3DBlob (aka ID3D10Blob) minimal implementation.
//
// dxvk-native only vendors D3DCreateBlob's declaration (directx/d3dcompiler.h)
// - its Meson build (which would define it, see d3d10/d3d10_main.cpp calling
// it) is not wired into this CMake tree (headers-only usage, see
// xrRenderPC_R4/CMakeLists.txt), so there is nothing to link against. This is
// a self-contained stand-in used only by D3DX11SaveTextureToMemory below.
//-----------------------------------------------------------------------------
class D3DX11CompatBlob : public ID3DBlob
{
public:
	explicit D3DX11CompatBlob(size_t size) : m_ref(1), m_data(size)
	{
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (!ppvObject)
			return E_POINTER;
		if (riid == IID_ID3D10Blob || riid == IID_IUnknown)
		{
			*ppvObject = static_cast<ID3DBlob*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }

	ULONG STDMETHODCALLTYPE Release() override
	{
		ULONG ref = --m_ref;
		if (ref == 0)
			delete this;
		return ref;
	}

	void* STDMETHODCALLTYPE GetBufferPointer() override { return m_data.data(); }
	SIZE_T STDMETHODCALLTYPE GetBufferSize() override { return m_data.size(); }

private:
	ULONG m_ref;
	std::vector<uint8_t> m_data;
};

//-----------------------------------------------------------------------------
// D3DX11LoadTextureFromTexture
//
// Only the subset needed by r__screenshot.cpp's SM_FOR_GAMESAVE/
// SM_FOR_MPSENDING paths is implemented: resizing (and, when the destination
// is block-compressed, compressing) a full-res render-target capture down
// into an already-created destination texture. pLoadInfo is unused - every
// call site in this codebase passes NULL.
//-----------------------------------------------------------------------------
inline HRESULT D3DX11LoadTextureFromTexture(ID3D11DeviceContext* pContext, ID3D11Resource* pSrcTexture,
                                             void* /*pLoadInfo*/, ID3D11Resource* pDstTexture)
{
	ID3D11Device* device = nullptr;
	pContext->GetDevice(&device);
	if (!device)
		return E_FAIL;

	ID3D11Texture2D* dstTex2D = nullptr;
	HRESULT hr = pDstTexture->QueryInterface(IID_ID3D11Texture2D, reinterpret_cast<void**>(&dstTex2D));
	if (FAILED(hr))
	{
		device->Release();
		return hr;
	}
	D3D11_TEXTURE2D_DESC dstDesc;
	dstTex2D->GetDesc(&dstDesc);

	DirectX::ScratchImage captured;
	hr = DirectX::CaptureTexture(device, pContext, pSrcTexture, captured);
	device->Release();
	if (FAILED(hr))
	{
		dstTex2D->Release();
		return hr;
	}

	const DirectX::TexMetadata& srcMeta = captured.GetMetadata();

	DirectX::ScratchImage resized;
	const DirectX::ScratchImage* pResized = &captured;
	if (srcMeta.width != dstDesc.Width || srcMeta.height != dstDesc.Height)
	{
		hr = DirectX::Resize(captured.GetImages(), captured.GetImageCount(), srcMeta, dstDesc.Width, dstDesc.Height,
		                      DirectX::TEX_FILTER_DEFAULT, resized);
		if (FAILED(hr))
			return hr;
		pResized = &resized;
	}

	DirectX::ScratchImage converted;
	const DirectX::ScratchImage* pFinal = pResized;
	if (pResized->GetMetadata().format != dstDesc.Format)
	{
		const DirectX::TexMetadata& meta = pResized->GetMetadata();
		if (DirectX::IsCompressed(dstDesc.Format))
			hr = DirectX::Compress(device, pResized->GetImages(), pResized->GetImageCount(), meta, dstDesc.Format,
			                        DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
		else
			hr = DirectX::Convert(pResized->GetImages(), pResized->GetImageCount(), meta, dstDesc.Format,
			                       DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
		if (FAILED(hr))
		{
			dstTex2D->Release();
			return hr;
		}
		pFinal = &converted;
	}

	const DirectX::Image* img = pFinal->GetImage(0, 0, 0);
	if (!img)
	{
		dstTex2D->Release();
		return E_FAIL;
	}
	pContext->UpdateSubresource(dstTex2D, 0, nullptr, img->pixels, static_cast<UINT>(img->rowPitch),
	                             static_cast<UINT>(img->slicePitch));
	dstTex2D->Release();
	return S_OK;
}

//-----------------------------------------------------------------------------
// D3DX11SaveTextureToMemory
//
// Always encodes DDS - see the file-header comment above for why non-DDS
// requests silently degrade to DDS instead of failing with E_NOTIMPL.
//-----------------------------------------------------------------------------
inline HRESULT D3DX11SaveTextureToMemory(ID3D11DeviceContext* pContext, ID3D11Resource* pSrcTexture,
                                          D3DX11_IMAGE_FILE_FORMAT DestFormat, ID3DBlob** ppDestBuf, UINT /*Flags*/)
{
	if (!ppDestBuf)
		return E_POINTER;

	if (DestFormat != D3DX11_IFF_DDS)
		Msg("! D3DX11SaveTextureToMemory: no WIC on this platform - saving as DDS instead of format %d", DestFormat);

	ID3D11Device* device = nullptr;
	pContext->GetDevice(&device);
	if (!device)
		return E_FAIL;

	DirectX::ScratchImage image;
	HRESULT hr = DirectX::CaptureTexture(device, pContext, pSrcTexture, image);
	device->Release();
	if (FAILED(hr))
		return hr;

	DirectX::Blob ddsBlob;
	const DirectX::TexMetadata& meta = image.GetMetadata();
	if (meta.mipLevels == 1 && meta.arraySize == 1)
		hr = DirectX::SaveToDDSMemory(*image.GetImage(0, 0, 0), DirectX::DDS_FLAGS_NONE, ddsBlob);
	else
		hr = DirectX::SaveToDDSMemory(image.GetImages(), image.GetImageCount(), meta, DirectX::DDS_FLAGS_NONE, ddsBlob);
	if (FAILED(hr))
		return hr;

	D3DX11CompatBlob* blob = new D3DX11CompatBlob(ddsBlob.GetBufferSize());
	std::memcpy(blob->GetBufferPointer(), ddsBlob.GetBufferPointer(), ddsBlob.GetBufferSize());
	*ppDestBuf = blob;
	return S_OK;
}

#endif // D3DX11TEX_COMPAT_H
