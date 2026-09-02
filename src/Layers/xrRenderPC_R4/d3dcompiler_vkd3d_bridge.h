// Primitive-only C bridge to the vendored vkd3d-shader library (see
// src/3rd party/vkd3d-shader/CMakeLists.txt for provenance). This header
// is included from BOTH sides of the D3DCompiler compat shim:
//
//   - d3dcompiler_vkd3d_bridge.c (the implementation): includes ONLY
//     vkd3d-shader's own headers.
//   - d3dcompiler_shim.cpp (the D3D-facing shim): includes ONLY dxvk's
//     D3D11/d3dcompiler headers.
//
// It must therefore never be allowed to drag either side's headers into
// the other translation unit - dxvk's D3D11 headers and vkd3d-shader's
// own public headers both define things like HRESULT/BOOL/UINT with
// incompatible typedefs, and mixing them in one TU does not compile.
// Every type crossing this boundary is a C primitive (void*, size_t,
// int, const char*) or a POD struct defined right here - never a type
// from either side's own headers.
#ifndef XR_D3DCOMPILER_VKD3D_BRIDGE_H
#define XR_D3DCOMPILER_VKD3D_BRIDGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// DXBC container section fourCC tags this bridge cares about, expressed
// as plain numeric constants (little-endian packed ASCII, matching
// vkd3d-shader's own VKD3D_MAKE_TAG('X','X','X','X') convention in
// include/private/vkd3d_common.h) so callers never need to see that
// header.
#define XR_VKD3D_BRIDGE_TAG_ISGN 0x4e475349u /* "ISGN" */
#define XR_VKD3D_BRIDGE_TAG_RDEF 0x46454452u /* "RDEF" */

// Matrix-packing compile flags for xr_vkd3d_bridge_compile_hlsl's `flags`
// parameter, expressed as bridge-local bits (deliberately not reusing
// D3DCOMPILE_PACK_MATRIX_*'s own numeric values, even though they happen
// to already match, to keep this header independent of dxvk's
// d3dcompiler.h). At most one of these should be set; if neither is set,
// vkd3d-shader's own HLSL-frontend default (column-major) applies.
#define XR_VKD3D_BRIDGE_PACK_MATRIX_ROW_MAJOR    0x1u
#define XR_VKD3D_BRIDGE_PACK_MATRIX_COLUMN_MAJOR 0x2u

// A single HLSL preprocessor macro (name/definition pair), mirroring
// D3D_SHADER_MACRO's layout without depending on dxvk's header for it.
struct xr_vkd3d_bridge_macro
{
	const char *name;
	const char *value;
};

// Callback types mirroring ID3DInclude::Open/Close, expressed in
// primitive terms only. `local` is nonzero for a double-quoted #include,
// zero for an angle-bracketed one. On success the Open callback must
// fill *out_data/*out_size and return 0; a nonzero return means failure.
typedef int (*xr_vkd3d_bridge_open_include_fn)(
	const char *filename, int local, const char *parent_data,
	void *context, const void **out_data, size_t *out_size);
typedef void (*xr_vkd3d_bridge_close_include_fn)(
	const void *data, size_t size, void *context);

// Compiles HLSL source (source/source_len) into a DXBC-TPF container
// (matching D3DCompile's byte-code output). On success returns 0 and
// sets *out_code/*out_size to a malloc'd buffer (free with
// xr_vkd3d_bridge_free); on failure returns nonzero and, if messages
// were produced, sets *out_messages to a malloc'd NUL-terminated string
// (free with xr_vkd3d_bridge_free_messages).
// `flags` is a bitmask of XR_VKD3D_BRIDGE_PACK_MATRIX_* above.
int xr_vkd3d_bridge_compile_hlsl(
	const char *source, size_t source_len, const char *source_name,
	const struct xr_vkd3d_bridge_macro *macros, unsigned int macro_count,
	xr_vkd3d_bridge_open_include_fn open_include,
	xr_vkd3d_bridge_close_include_fn close_include, void *include_context,
	const char *entry_point, const char *profile, unsigned int flags,
	void **out_code, size_t *out_size, char **out_messages);

// Disassembles a DXBC-TPF container into D3D assembly text (matching
// D3DDisassemble's output). *out_code is malloc'd (free with
// xr_vkd3d_bridge_free) and NUL-terminated.
int xr_vkd3d_bridge_disassemble(
	const void *dxbc, size_t dxbc_size,
	void **out_code, size_t *out_size, char **out_messages);

// Extracts a single named section (by fourCC `tag`, one of the
// XR_VKD3D_BRIDGE_TAG_* constants above) out of a DXBC container and
// re-wraps it alone into a fresh, correctly-checksummed minimal DXBC
// container. Used by D3DGetInputSignatureBlob to produce a
// self-contained ISGN-only blob without hand-rolling DXBC container
// framing/checksums. *out_code is malloc'd (free with
// xr_vkd3d_bridge_free). Returns nonzero (with no allocation) if the
// requested section is not present.
int xr_vkd3d_bridge_extract_dxbc_section(
	const void *dxbc, size_t dxbc_size, unsigned int tag,
	void **out_code, size_t *out_size);

// Copies out the raw bytes of a single named section (by fourCC `tag`)
// without re-wrapping it in a container - used by D3DReflect to get at
// the raw RDEF chunk bytes for hand-rolled parsing. *out_data is
// malloc'd (free with xr_vkd3d_bridge_free). Returns nonzero (with no
// allocation) if the requested section is not present.
int xr_vkd3d_bridge_find_dxbc_section(
	const void *dxbc, size_t dxbc_size, unsigned int tag,
	void **out_data, size_t *out_size);

// Frees a buffer returned via any out_code/out_data parameter above.
void xr_vkd3d_bridge_free(void *p);

// Frees a message string returned via any out_messages parameter above.
void xr_vkd3d_bridge_free_messages(char *messages);

#ifdef __cplusplus
}
#endif

#endif /* XR_D3DCOMPILER_VKD3D_BRIDGE_H */
