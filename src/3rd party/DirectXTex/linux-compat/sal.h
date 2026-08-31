#pragma once

// Stand-in for the Windows SDK's <sal.h> (Source-code Annotation Language).
// These are purely static-analysis hints in real Windows headers - dropping
// them is semantically identical to a real Windows build. This file exists
// only because DirectXMath.h (src/3rd party/DirectXMath/Inc/DirectXMath.h)
// unconditionally #include "sal.h" with no _WIN32/__has_include guard.
// Covers every SAL macro actually referenced across DirectXMath/DirectXTex
// (see XR_DIRECTXTEX_DXVK_NATIVE); not a complete reimplementation of SAL.

#define _In_
#define _In_z_
#define _In_opt_
#define _Inout_
#define _Inout_opt_
#define _Out_
#define _Out_opt_
#define _Outptr_
#define _Reserved_
#define _Use_decl_annotations_
#define _PREFAST_
#define _COM_Outptr_

#define _In_count_(...)
#define _In_range_(...)
#define _In_reads_(...)
#define _In_reads_opt_(...)
#define _In_reads_bytes_(...)
#define _Inout_updates_all_(...)
#define _Inout_updates_all_opt_(...)
#define _Inout_updates_bytes_(...)
#define _Out_writes_(...)
#define _Out_writes_all_(...)
#define _Out_writes_bytes_(...)
#define _Out_writes_bytes_to_opt_(...)
#define _Out_writes_opt_(...)
#define _Analysis_assume_(...)
#define _Success_(...)
#define _When_(...)
