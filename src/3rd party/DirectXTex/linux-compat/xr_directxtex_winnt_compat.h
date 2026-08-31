#pragma once

// Two winnt.h/windef.h pieces dxvk-native's own Windows-header compat layer
// (src/3rd party/dxvk-native/include/native/windows/windows_base.h) doesn't
// provide, because dxvk-native itself is normally built with a MinGW
// cross-compiler - where __cdecl etc. are real compiler-recognized calling-
// convention keywords, not macros needing a stub - rather than native Linux
// GCC/Clang, which don't recognize them as keywords at all. windows_base.h
// already stubs __stdcall/WINAPI/STDMETHODCALLTYPE to empty for exactly this
// reason; __cdecl and DEFINE_ENUM_FLAG_OPERATORS are the two additional ones
// DirectXTex's own headers (DirectXTexP.h/DirectXTex.h/DirectXTex.inl) use.

#include <type_traits>

#ifndef __cdecl
#define __cdecl
#endif

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

// HRESULT codes dxvk-native's windows_base.h doesn't define (it only
// provides the handful its own D3D/DXGI implementation needs).
#ifndef E_ABORT
#define E_ABORT ((HRESULT)0x80004004)
#endif
#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000B)
#endif
#ifndef E_UNEXPECTED
#define E_UNEXPECTED ((HRESULT)0x8000FFFF)
#endif
#ifndef E_NOT_SUFFICIENT_BUFFER
#define E_NOT_SUFFICIENT_BUFFER ((HRESULT)0x8007007A)
#endif
#ifndef FACILITY_WIN32
#define FACILITY_WIN32 7
#endif
#ifndef HRESULT_FROM_WIN32
#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT)(((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))
#endif

// Reproduction of the real winnt.h macro of the same name (defines the
// bitwise operators for a scoped/unscoped enum used as a flags type).
#ifndef DEFINE_ENUM_FLAG_OPERATORS
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline constexpr ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) | ((std::underlying_type_t<ENUMTYPE>)b)); } \
inline ENUMTYPE& operator |= (ENUMTYPE &a, ENUMTYPE b) noexcept { return (ENUMTYPE&)(((std::underlying_type_t<ENUMTYPE>&)a) |= ((std::underlying_type_t<ENUMTYPE>)b)); } \
inline constexpr ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) & ((std::underlying_type_t<ENUMTYPE>)b)); } \
inline ENUMTYPE& operator &= (ENUMTYPE &a, ENUMTYPE b) noexcept { return (ENUMTYPE&)(((std::underlying_type_t<ENUMTYPE>&)a) &= ((std::underlying_type_t<ENUMTYPE>)b)); } \
inline constexpr ENUMTYPE operator ~ (ENUMTYPE a) noexcept { return ENUMTYPE(~((std::underlying_type_t<ENUMTYPE>)a)); } \
inline constexpr ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) ^ ((std::underlying_type_t<ENUMTYPE>)b)); } \
inline ENUMTYPE& operator ^= (ENUMTYPE &a, ENUMTYPE b) noexcept { return (ENUMTYPE&)(((std::underlying_type_t<ENUMTYPE>&)a) ^= ((std::underlying_type_t<ENUMTYPE>)b)); } \
}
#endif
