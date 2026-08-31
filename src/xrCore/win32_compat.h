#pragma once

// Portable stand-ins for the Win32 types/constants and CRT "secure"/
// underscore-prefixed functions this codebase assumes are always
// available (normally pulled in transitively via windows.h, which the
// Linux build doesn't include - see xrCore_platform.h). Real API-backed
// functionality (file mapping, critical sections, perf counters,
// directory-change notification) lives in the other posix_*.h headers
// alongside this one, not here - this file is just the "make the symbol
// exist" layer for basic types/constants/CRT-function renames.
//
// See playground/xray-monolith-vulkan-port-notes.md section 14.

#include <alloca.h>
#include <cerrno>
#include <cstdarg>
#include <climits> // PATH_MAX (GetFullPathName)
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iconv.h>
#include <malloc.h>
#include <map>
#include <pthread.h>
#include <pwd.h>
#include <sched.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <vector>

// --- Basic integer typedefs -------------------------------------------
// DWORD must be exactly 32 bits on every platform (it's Windows' "32-bit
// unsigned" type, LLP64-defined) - `unsigned long` is 32-bit on Windows
// but 64-bit on Linux/LP64, so unsigned int is the only correct portable
// choice here, not a straight "looks equivalent" substitution.
using BYTE = unsigned char;
using LPBYTE = unsigned char*;
using WORD = unsigned short;
using DWORD = unsigned int;
using LPDWORD = unsigned int*;
using LPWORD = unsigned short*;
#define CALLBACK // __stdcall calling-convention marker, no-op on x86-64 (single calling convention)
// `interface` - MSVC/COM's `#define interface struct` (from <unknwn.h>,
// pulled in transitively by <windows.h>), used in this codebase as a
// convention for pure-abstract-base classes (first hit: xrGame's
// ph_shell_interface.h). Never previously needed since no earlier module
// used the convention.
#define interface struct
// `IN`/`OUT` - Windows SDK <windef.h> parameter-direction annotation macros,
// both defined to expand to nothing (pure documentation, no semantic effect).
// First hit: xrGame's ai/monsters/basemonster/base_monster.h's
// `get_debug_var(pcstr var_name, OUT Type& result)` - genuinely relied on
// this expanding to nothing (MSVC's real <windef.h> is transitively visible
// there via the Windows SDK headers this codebase's PCH otherwise pulls in);
// GCC has no such macro at all, so it parsed `OUT Type& result` as two
// separate declarators instead of one, both nonsensical.
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
// DllMain() reason-for-call codes. Only ever reach a switch() body that's
// unreachable dead code on a static (non-DLL) build, but the constants
// themselves still need to exist for such code to compile at all.
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3
#define DLL_PROCESS_DETACH 0
// LONG/ULONG are LLP64 types - always 32-bit on real Windows even on 64-bit
// builds, same reasoning as DWORD above. Must be int32_t/uint32_t (not
// `long`, which is 64-bit on Linux/LP64) both for genuine Win32-ABI
// correctness and because dxvk-native's own windows_base.h (pulled in via
// d3d9.h/d3d11.h in xrRenderPC_R4) independently defines these as
// int32_t/uint32_t too - matching types make its redundant typedefs
// harmless duplicate declarations instead of conflicting ones.
using LONG = int32_t;
using ULONG = uint32_t;
using BOOL = int;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
using UINT_PTR = uintptr_t;
using DWORD_PTR = uintptr_t;
using ULONG_PTR = uintptr_t;
using INT_PTR = intptr_t;
// __int64 - an MSVC extended-integer *keyword* (built into the compiler, no
// header needed there), used directly as a type name in several source
// files for pointer<->integer round-trips (e.g. `(void*)(__int64)data` /
// `(int)(__int64)itm->GetData()`, first hit: xrGame/ui/UIComboBox.cpp).
// GCC/Clang have no such keyword - xrCore's own _types.h already
// established s64/u64 as the portable replacement typedefs for exactly this
// MSVC-ism, but existing call sites still spell it `__int64` directly. A
// plain `#define` (rather than `using __int64 = long long;`) is needed
// specifically so `unsigned __int64` (also used elsewhere in this tree,
// e.g. xrCore/rt_lzoconf.h) still parses too - `unsigned` can only combine
// with a builtin type-specifier grammatically, not with a typedef-name, so
// a `using`-alias would silently break that combination while a textual
// macro survives it (`unsigned __int64` -> `unsigned long long`).
#define __int64 long long
using PSTR = char*;
using LPVOID = void*;
using PVOID = void*;
using VOID = void;

// HRESULT - the small subset actually used in this tree (xrNetServer's
// message-handler-shaped return codes, never real COM error codes since
// nothing here talks to real COM anymore). Standard values/macros, not a
// behavior shim. int32_t (not `long`) for the same LLP64 reason as LONG
// above - also keeps it a harmless duplicate, not a conflict, against
// dxvk-native's windows_base.h HRESULT typedef.
using HRESULT = int32_t;
#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

inline unsigned long GetCurrentThreadId()
{
	return static_cast<unsigned long>(pthread_self());
}

// WAVEFORMATEX - standard <mmsystem.h>/<mmreg.h> WAV format header
// (xrSound stores one per loaded sound as SoundRender_Source::m_wformat).
// Fixed, well-documented binary layout - not a Windows-behavior shim,
// just the struct definition itself.
struct WAVEFORMATEX
{
	WORD wFormatTag;
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD nBlockAlign;
	WORD wBitsPerSample;
	WORD cbSize;
};
#define WAVE_FORMAT_PCM 1

// D3DCOLOR_RGBA - real, well-documented d3d9types.h macro (ARGB byte
// packing), not tied to any live D3D object - used here purely as a
// color-packing helper (xr_ioc_cmd.h's console-variable-to-Fvector4
// binding), safe to keep as-is once the renderer becomes Vulkan. Real
// d3d9types.h defines D3DCOLOR as `typedef DWORD D3DCOLOR;` (DWORD is
// always 32-bit) - uint32_t here (not `unsigned long`) matches that exactly,
// making dxvk-native's own d3d9types.h D3DCOLOR typedef (pulled in via
// xrRenderPC_R4's d3d9.h include) a harmless duplicate instead of a
// conflicting redeclaration.
using D3DCOLOR = uint32_t;
#define D3DCOLOR_RGBA(r, g, b, a) \
	((D3DCOLOR)((((a)&0xff) << 24) | (((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff)))
// D3DCOLOR_XRGB - same d3d9types.h family, alpha forced to opaque (0xff).
#define D3DCOLOR_XRGB(r, g, b) D3DCOLOR_RGBA(r, g, b, 255)
// D3DCOLOR_ARGB - same d3d9types.h family, same byte packing as
// D3DCOLOR_RGBA just with the argument order swapped (alpha first, matching
// real d3d9types.h) - first real call site: xrGame/imotion_position.cpp's
// debug-draw color literals.
#define D3DCOLOR_ARGB(a, r, g, b) D3DCOLOR_RGBA(r, g, b, a)

// __min/__max - real MSVC CRT macros (<stdlib.h>, non-standard) forwarding
// to the project's own portable _min<T>/_max<T> templates
// (_std_extensions.h, always in scope by the time these are actually used -
// macros expand at use site, not definition site). First real call site:
// xrGame/CarDrone.cpp.
#define __min(a, b) _min(a, b)
#define __max(a, b) _max(a, b)

// RGB - real wingdi.h macro (0x00bbggrr packing), used here purely as a
// color-packing helper for default-argument values (e.g.
// IGame_Level.h's CServerInfo::AddItem), not tied to any live GDI object.
#define RGB(r, g, b) \
	((unsigned long)((unsigned char)(r) | ((unsigned short)(unsigned char)(g) << 8) | (((unsigned long)(unsigned char)(b)) << 16)))

#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif

// itoa: MSVC CRT integer-to-string with arbitrary radix (this codebase's
// one call site uses base 16). No POSIX equivalent (only base-10 via
// sprintf); real minimal implementation, not just base-10 special-cased.
inline char* itoa(int value, char* buf, int radix)
{
	if (radix == 10)
	{
		std::sprintf(buf, "%d", value);
		return buf;
	}
	unsigned int uvalue = static_cast<unsigned int>(value);
	bool negative = false;
	if (radix == 10 && value < 0)
	{
		negative = true;
		uvalue = static_cast<unsigned int>(-value);
	}
	char tmp[34];
	int i = 0;
	if (uvalue == 0)
		tmp[i++] = '0';
	while (uvalue != 0)
	{
		int digit = uvalue % radix;
		tmp[i++] = (digit < 10) ? static_cast<char>('0' + digit) : static_cast<char>('a' + digit - 10);
		uvalue /= static_cast<unsigned int>(radix);
	}
	int j = 0;
	if (negative)
		buf[j++] = '-';
	while (i > 0)
		buf[j++] = tmp[--i];
	buf[j] = '\0';
	return buf;
}

// --- CPU topology (cpuid.cpp's hyperthreading/physical-core detection) ---
// Real /proc/cpuinfo + sched_getaffinity-backed implementation, not a
// stub - genuinely different mechanism from Win32's (GetLogicalProcessorInformation
// returns one entry per *physical core* with a ProcessorMask of its
// logical siblings; here that's derived by grouping /proc/cpuinfo's
// "processor" entries by their (physical id, core id) pair). Only the
// two fields this codebase's one call site actually reads
// (Relationship, ProcessorMask) are modeled - the real Win32 struct's
// cache/NUMA union members aren't needed.
struct SYSTEM_LOGICAL_PROCESSOR_INFORMATION
{
	ULONG_PTR ProcessorMask;
	int Relationship;
};
using PSYSTEM_LOGICAL_PROCESSOR_INFORMATION = SYSTEM_LOGICAL_PROCESSOR_INFORMATION*;
enum { RelationProcessorCore = 0 };

inline void* GetCurrentProcess() { return nullptr; } // pseudo-handle, value unused by any of our shims

// Win32 SwitchToThread() yields the rest of the calling thread's timeslice
// to another ready thread, returning nonzero iff it actually switched -
// sched_yield() is the direct POSIX equivalent.
inline BOOL SwitchToThread() { return sched_yield() == 0 ? TRUE : FALSE; }

// Atomic exchange, returns the previous value - GCC/Clang's
// __sync_lock_test_and_set builtin has identical semantics for this
// exact use (a spinlock test-and-set), so no need to migrate the call
// site to std::atomic.
inline LONG InterlockedExchange(volatile LONG* target, LONG value)
{
	return __sync_lock_test_and_set(target, value);
}

inline bool GetProcessAffinityMask(void* /*process*/, ULONG_PTR* processMask, ULONG_PTR* systemMask)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	if (sched_getaffinity(0, sizeof(set), &set) != 0)
		return false;
	ULONG_PTR mask = 0;
	long nproc = sysconf(_SC_NPROCESSORS_CONF);
	for (long i = 0; i < nproc && i < static_cast<long>(sizeof(ULONG_PTR) * 8); ++i)
		if (CPU_ISSET(i, &set))
			mask |= (ULONG_PTR(1) << i);
	*processMask = mask;
	*systemMask = mask;
	return true;
}

inline bool GetLogicalProcessorInformation(SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer, DWORD* returnedLength)
{
	std::vector<std::pair<int, int>> logical_to_corekey;
	FILE* f = std::fopen("/proc/cpuinfo", "r");
	if (f)
	{
		char line[256];
		int cur_proc = -1, cur_phys = 0, cur_core = -1;
		auto flush = [&]()
		{
			if (cur_proc >= 0)
			{
				if (static_cast<int>(logical_to_corekey.size()) <= cur_proc)
					logical_to_corekey.resize(cur_proc + 1, {-1, -1});
				logical_to_corekey[cur_proc] = {cur_phys, (cur_core >= 0) ? cur_core : cur_proc};
			}
		};
		while (std::fgets(line, sizeof(line), f))
		{
			int v;
			if (std::sscanf(line, "processor : %d", &v) == 1 || std::sscanf(line, "processor: %d", &v) == 1)
			{
				flush();
				cur_proc = v;
				cur_phys = 0;
				cur_core = -1;
			}
			else if (std::sscanf(line, "physical id : %d", &v) == 1 || std::sscanf(line, "physical id: %d", &v) == 1)
				cur_phys = v;
			else if (std::sscanf(line, "core id : %d", &v) == 1 || std::sscanf(line, "core id: %d", &v) == 1)
				cur_core = v;
		}
		flush();
		std::fclose(f);
	}

	std::map<std::pair<int, int>, ULONG_PTR> core_masks;
	for (size_t lcpu = 0; lcpu < logical_to_corekey.size(); ++lcpu)
	{
		auto key = logical_to_corekey[lcpu];
		if (key.first < 0)
			continue;
		if (lcpu < sizeof(ULONG_PTR) * 8)
			core_masks[key] |= (ULONG_PTR(1) << lcpu);
	}
	if (core_masks.empty())
	{
		// /proc/cpuinfo parse failed entirely - fall back to one "core"
		// per online logical CPU (no hyperthreading detection, but still
		// a correct logical-processor count).
		long n = sysconf(_SC_NPROCESSORS_ONLN);
		for (long i = 0; i < n && i < static_cast<long>(sizeof(ULONG_PTR) * 8); ++i)
			core_masks[{0, static_cast<int>(i)}] = (ULONG_PTR(1) << i);
	}

	DWORD needed = static_cast<DWORD>(core_masks.size() * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
	if (!buffer || *returnedLength < needed)
	{
		*returnedLength = needed;
		return false; // matches Win32: false + required size when buffer is too small/null
	}

	size_t i = 0;
	for (auto& kv : core_masks)
	{
		buffer[i].ProcessorMask = kv.second;
		buffer[i].Relationship = RelationProcessorCore;
		++i;
	}
	*returnedLength = needed;
	return true;
}

// --- CRT "secure" (_s suffix) functions ---------------------------------
using errno_t = int;

// Real sscanf_s takes an extra size_t per %s/%c conversion; every call
// site in this tree only uses numeric conversions (%u/%d/etc, verified
// per call site before adding), where sscanf_s and plain sscanf behave
// identically - a straight alias, not a general-purpose sscanf_s shim.
#define sscanf_s sscanf

inline errno_t strncpy_s(char* dest, size_t destsz, const char* src, size_t count)
{
	if (!dest || destsz == 0)
		return EINVAL;
	size_t n = (count == static_cast<size_t>(-1)) ? destsz - 1 : count;
	if (n >= destsz)
		n = destsz - 1;
	std::strncpy(dest, src, n);
	dest[n] = '\0';
	return 0;
}
// MSVC's real <string.h> also declares a 3-argument array-deducing template
// overload (under _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES, always active for
// this codebase) that infers destsz from a fixed-size char array destination -
// used pervasively wherever the destination is a string32/string256/string512-
// style typedef (first real call site: xrGame/ui/Restrictions.cpp). Same
// array-deducing-overload pattern this header already uses for vsprintf_s
// below.
template <size_t N>
inline errno_t strncpy_s(char (&dest)[N], const char* src, size_t count)
{
	return strncpy_s(dest, N, src, count);
}

// Same shape as strncpy_s above (first real call site: xrGame/ui/UITalkWnd.cpp,
// building a save-slot filename by appending an extension) - a straight,
// bounded, always-null-terminated implementation of MSVC's real strncat_s.
inline errno_t strncat_s(char* dest, size_t destsz, const char* src, size_t count)
{
	if (!dest || destsz == 0)
		return EINVAL;
	size_t dest_len = std::strlen(dest);
	if (dest_len >= destsz)
		return EINVAL;
	size_t avail = destsz - dest_len - 1;
	size_t n = (count == static_cast<size_t>(-1)) ? avail : count;
	if (n > avail)
		n = avail;
	std::strncat(dest, src, n);
	return 0;
}
template <size_t N>
inline errno_t strncat_s(char (&dest)[N], const char* src, size_t count)
{
	return strncat_s(dest, N, src, count);
}

inline void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
	// MSVC's _splitpath has 4 independently-sized output buffers (no sizes
	// passed); this codebase always sizes them string_path/similar, ample
	// for real paths.
	if (drive) drive[0] = '\0';

	const char* last_slash = path;
	for (const char* p = path; *p; ++p)
		if (*p == '/' || *p == '\\')
			last_slash = p + 1;

	if (dir)
	{
		size_t n = last_slash - path;
		std::memcpy(dir, path, n);
		dir[n] = '\0';
	}

	const char* dot = nullptr;
	for (const char* p = last_slash; *p; ++p)
		if (*p == '.')
			dot = p;

	const char* name_end = dot ? dot : (last_slash + std::strlen(last_slash));

	if (fname)
	{
		size_t n = name_end - last_slash;
		std::memcpy(fname, last_slash, n);
		fname[n] = '\0';
	}
	if (ext)
	{
		if (dot)
			std::strcpy(ext, dot);
		else
			ext[0] = '\0';
	}
}

inline errno_t _splitpath_s(const char* path, char* drive, size_t, char* dir, size_t, char* fname, size_t,
                             char* ext, size_t)
{
	_splitpath(path, drive, dir, fname, ext);
	return 0;
}

inline errno_t _i64toa_s(long long value, char* buffer, size_t bufsize, int radix)
{
	if (radix == 10)
		std::snprintf(buffer, bufsize, "%lld", value);
	else
		std::snprintf(buffer, bufsize, "%llx", value);
	return 0;
}

inline errno_t _ui64toa_s(unsigned long long value, char* buffer, size_t bufsize, int radix)
{
	if (radix == 10)
		std::snprintf(buffer, bufsize, "%llu", value);
	else
		std::snprintf(buffer, bufsize, "%llx", value);
	return 0;
}

// --- CRT underscore-prefixed functions with plain POSIX equivalents -----
inline char* strlwr(char* s)
{
	for (char* p = s; *p; ++p)
		*p = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
	return s;
}
inline char* _strlwr(char* s) { return strlwr(s); }
inline char* strupr(char* s)
{
	for (char* p = s; *p; ++p)
		*p = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
	return s;
}
inline char* _strupr(char* s) { return strupr(s); }
inline int stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
inline int _stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
inline int strcmpi(const char* a, const char* b) { return strcasecmp(a, b); }
inline long long _atoi64(const char* s) { return std::atoll(s); }
inline unsigned long long _strtoui64(const char* s, char** end, int base) { return std::strtoull(s, end, base); }
inline int _vsnprintf(char* buf, size_t n, const char* fmt, va_list args) { return vsnprintf(buf, n, fmt, args); }
#define _snprintf snprintf
inline int vsnprintf_s(char* dest, size_t destsz, size_t count, const char* fmt, va_list args)
{
	size_t n = (count < destsz) ? count : destsz - 1;
	return vsnprintf(dest, n + 1, fmt, args);
}
template <size_t N>
inline int vsprintf_s(char (&dest)[N], const char* fmt, va_list args)
{
	return vsnprintf(dest, N, fmt, args);
}
inline int vsprintf_s(char* dest, size_t destsz, const char* fmt, va_list args)
{
	return vsnprintf(dest, destsz, fmt, args);
}
template <size_t N>
inline int sprintf_s(char (&dest)[N], const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int result = vsnprintf(dest, N, fmt, args);
	va_end(args);
	return result;
}

// _strdate/_strtime: legacy MSVC "DD/MM/YY"/"HH:MM:SS" formatters, used
// only for log/debug timestamps in this codebase - real localtime-based
// implementation, not a stub.
inline char* _strdate(char* buf)
{
	std::time_t t = std::time(nullptr);
	std::tm tmv{};
	localtime_r(&t, &tmv);
	std::snprintf(buf, 9, "%02d/%02d/%02d", tmv.tm_mon + 1, tmv.tm_mday, (tmv.tm_year + 1900) % 100);
	return buf;
}
inline char* _strtime(char* buf)
{
	std::time_t t = std::time(nullptr);
	std::tm tmv{};
	localtime_r(&t, &tmv);
	std::snprintf(buf, 9, "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
	return buf;
}
inline void _tzset() { tzset(); }

// _time64/_localtime64 - MSVC CRT's explicitly-64-bit time functions
// (from an era when plain time_t/time()/localtime() could still be
// 32-bit on some MSVC targets). glibc's time_t is already 64-bit on
// this platform, so the portable equivalents are just time()/localtime()
// - a real semantic match, not a stub. xr_dsa_signer.cpp is the first
// file in this port to need it.
inline std::time_t _time64(std::time_t* dest) { return std::time(dest); }
inline std::tm* _localtime64(const std::time_t* t) { return std::localtime(t); }

// _time32/__time32_t - the 32-bit-explicit counterpart of _time64 above,
// same rationale: real Win32 __time32_t is always a 32-bit signed count of
// seconds, so int32_t is a genuine semantic match, not a stub.
using __time32_t = int32_t;
inline __time32_t _time32(__time32_t* dest)
{
	std::time_t t = std::time(nullptr);
	if (dest)
		*dest = static_cast<__time32_t>(t);
	return static_cast<__time32_t>(t);
}

// timeGetTime() - <mmsystem.h>'s millisecond tick counter (device.cpp's
// CRenderDevice::Run()/mt_Thread timer-delta calibration wants "some
// monotonic millisecond clock", not wall time - it only ever diffs two
// consecutive readings). CLOCK_MONOTONIC is the direct portable
// equivalent, real implementation not a stub. Deliberately implemented
// here rather than pulled from SDL_GetTicks() - xrCore has no SDL2
// dependency and shouldn't gain one just for this.
inline unsigned long timeGetTime()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return static_cast<unsigned long>(ts.tv_sec) * 1000UL + static_cast<unsigned long>(ts.tv_nsec) / 1000000UL;
}

// GetTickCount() - <windows.h>'s millisecond tick counter (game_sv_event_queue.cpp
// wants "milliseconds since some fixed point, monotonic" for its own
// stale-event pruning - it only ever diffs against a stored prior reading,
// same usage shape as timeGetTime() above). Functionally identical Win32
// API, different header (<windows.h> vs <mmsystem.h>) - just forwards to
// the same CLOCK_MONOTONIC-backed implementation rather than duplicating it.
inline unsigned long GetTickCount() { return timeGetTime(); }

#define _alloca alloca

// _msize: real glibc equivalent exists (usable allocated size of a heap
// block). _expand: MSVC-specific "grow this block in place without
// moving it, or fail" - no POSIX equivalent, but every call site in this
// codebase already has a working fallback path for when it fails (falls
// back to malloc+copy), so always returning NULL (a legal _expand outcome)
// is a correct, if less optimal, implementation - not a stub that skips
// functionality, just always takes the already-implemented slow path.
inline size_t _msize(void* ptr) { return malloc_usable_size(ptr); }
inline void* _expand(void*, size_t) { return nullptr; }

// MSVC's _aligned_* CRT family - real posix_memalign()-backed
// implementation (distinct from this codebase's own xr_aligned_* pooled
// allocator, which is a separate thing living in xrMemory_align.cpp).
inline void* _aligned_malloc(size_t size, size_t alignment)
{
	if (alignment < sizeof(void*))
		alignment = sizeof(void*); // posix_memalign requires >= sizeof(void*)
	void* p = nullptr;
	return (posix_memalign(&p, alignment, size) == 0) ? p : nullptr;
}
inline void _aligned_free(void* ptr) { free(ptr); }
inline size_t _aligned_msize(void* ptr, size_t /*alignment*/, size_t /*offset*/) { return malloc_usable_size(ptr); }
inline void* _aligned_realloc(void* ptr, size_t size, size_t alignment)
{
	if (!ptr)
		return _aligned_malloc(size, alignment);
	if (size == 0)
	{
		free(ptr);
		return nullptr;
	}
	// No posix/glibc aligned-realloc - allocate fresh aligned block, copy,
	// free old (same approach _aligned_realloc's own contract requires
	// anyway: the returned pointer may differ from the input).
	size_t old_size = malloc_usable_size(ptr);
	void* p = _aligned_malloc(size, alignment);
	if (!p)
		return nullptr;
	memcpy(p, ptr, (old_size < size) ? old_size : size);
	free(ptr);
	return p;
}

// Real debugger-output API on Windows (visible in the attached debugger's
// output window); stderr is the direct portable equivalent for the same
// "developer sees this regardless of the normal log" purpose.
inline void OutputDebugString(const char* s) { std::fputs(s, stderr); }

// MessageBox needs a real GUI toolkit (GTK/Qt/SDL2) to show an actual
// dialog - none of which belong in xrCore (foundational, windowing-
// agnostic). This codebase only ever uses it for fatal-error/assert
// dialogs, so print to stderr and return as if the user dismissed it;
// revisit with a real SDL_ShowMessageBox() once there's a windowing
// layer to call it from.
#define MB_OK 0
#define MB_ICONERROR 0x10
#define IDOK 1
inline int MessageBox(void* /*hwnd*/, const char* text, const char* caption, unsigned /*type*/)
{
	std::fprintf(stderr, "[%s] %s\n", caption, text);
	return IDOK;
}

inline int _utime(const char* path, const utimbuf* times) { return utime(path, times); }

// --- System/user info (xrCore.cpp startup logging) ------------------------
inline DWORD GetCurrentDirectory(DWORD bufLen, char* buf)
{
	if (!getcwd(buf, bufLen))
		return 0;
	return static_cast<DWORD>(std::strlen(buf));
}

inline BOOL GetUserName(char* buf, DWORD* size)
{
	const char* name = getlogin();
	if (!name)
	{
		struct passwd* pw = getpwuid(getuid());
		name = pw ? pw->pw_name : "user";
	}
	std::strncpy(buf, name, *size - 1);
	buf[*size - 1] = '\0';
	*size = static_cast<DWORD>(std::strlen(buf));
	return TRUE;
}

inline BOOL GetComputerName(char* buf, DWORD* size)
{
	if (gethostname(buf, *size) != 0)
		return FALSE;
	*size = static_cast<DWORD>(std::strlen(buf));
	return TRUE;
}

// GetProcessHeap() is only used here for a %08x debug-log line
// ("Process heap 0x...") - no real Linux "process heap handle" concept
// to report, a fixed placeholder value is harmless.
inline void* GetProcessHeap() { return reinterpret_cast<void*>(1); }

// GetModuleHandle/GetModuleFileName - this codebase's one call site
// (xrCore.cpp startup) uses these together purely to find the running
// executable's own path (GetModuleHandle(MODULE_NAME) - the main module
// - immediately fed into GetModuleFileName). No real "loaded module"
// concept to model for a statically-linked Linux binary; /proc/self/exe
// gives the real, correct answer directly.
// GetFullPathName - resolves lpFileName to an absolute path via realpath()
// (first real call site: xrGame/ui/UIMapList.cpp's StartDedicatedServer(),
// a genuinely Win32-only "relaunch as a separate dedicated-server.exe
// process" flow - see notes file's established "dedicated server is
// Win32-only, not functionally portable" precedent; this stand-in exists
// so the surrounding code compiles, not to make that flow work on Linux).
// lpFilePart, if non-null, is set to point at the filename component
// within lpBuffer, matching the real Win32 API's contract.
inline DWORD GetFullPathName(const char* lpFileName, DWORD nBufferLength, char* lpBuffer, char** lpFilePart)
{
	char resolved[PATH_MAX];
	if (!realpath(lpFileName, resolved))
	{
		lpBuffer[0] = '\0';
		if (lpFilePart)
			*lpFilePart = nullptr;
		return 0;
	}
	size_t len = std::strlen(resolved);
	if (len >= nBufferLength)
		len = nBufferLength - 1;
	std::memcpy(lpBuffer, resolved, len);
	lpBuffer[len] = '\0';
	if (lpFilePart)
	{
		char* slash = std::strrchr(lpBuffer, '/');
		*lpFilePart = slash ? slash + 1 : lpBuffer;
	}
	return static_cast<DWORD>(len);
}

inline void* GetModuleHandle(const char*) { return nullptr; }
inline DWORD GetModuleFileName(void*, char* buf, DWORD size)
{
	ssize_t n = readlink("/proc/self/exe", buf, size - 1);
	if (n < 0)
	{
		buf[0] = '\0';
		return 0;
	}
	buf[n] = '\0';
	return static_cast<DWORD>(n);
}

// GetCommandLine() - real /proc/self/cmdline-backed implementation (used
// pervasively for -flag style engine startup switches: strstr(GetCommandLine(),
// "-editor") etc). /proc/self/cmdline is NUL-separated per-arg; joined with
// spaces here to match what every call site's strstr() substring search
// actually expects (Win32's GetCommandLine() returns one space-joined
// string, quoting aside).
inline const char* GetCommandLine()
{
	static std::string cached;
	if (cached.empty())
	{
		FILE* f = std::fopen("/proc/self/cmdline", "rb");
		if (f)
		{
			char buf[4096];
			size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
			std::fclose(f);
			for (size_t i = 0; i < n; ++i)
				cached += (buf[i] == '\0') ? ' ' : buf[i];
			while (!cached.empty() && cached.back() == ' ')
				cached.pop_back();
		}
	}
	return cached.c_str();
}

// IsDebuggerPresent() - real /proc/self/status-backed implementation
// (TracerPid is nonzero when attached to a debugger/strace/ptrace).
#ifndef _WIN32
inline void DebugBreak()
{
#if defined(__x86_64__) || defined(__i386__)
	__asm__ __volatile__("int3");
#else
	__builtin_trap();
#endif
}
#endif

inline bool IsDebuggerPresent()
{
	FILE* f = std::fopen("/proc/self/status", "r");
	if (!f)
		return false;
	char line[256];
	bool attached = false;
	while (std::fgets(line, sizeof(line), f))
	{
		if (std::strncmp(line, "TracerPid:", 10) == 0)
		{
			attached = std::atoi(line + 10) != 0;
			break;
		}
	}
	std::fclose(f);
	return attached;
}

// x87 FPU control word manipulation (denormal/precision control) - dead
// on x64 per the original engine's own #ifdef _M_AMD64 gating (see
// playground/xray-monolith-vulkan-port-notes.md section 5); these are
// only here so the never-taken x86 branches still parse.
using fpu_control_word_t = unsigned short;
inline void _control87(unsigned, unsigned) {}
inline void _clear87() {}
inline int _fpclass(double) { return 0; }
// _control87's mask/value arguments - meaningless since it's a no-op, just
// need distinct symbols for the (dead-on-x64, see _math.cpp) call sites to
// pass.
#define MCW_PC 0
#define MCW_RC 0
#define _PC_24 1
#define _PC_53 2
#define _PC_64 3
#define _RC_CHOP 1
#define _RC_NEAR 2
#define MCW_EM 0
#define _MCW_EM 0
#define _FPCLASS_SNAN 0
#define _FPCLASS_QNAN 1
#define _FPCLASS_NINF 2
#define _FPCLASS_PINF 3
#define _FPCLASS_ND 4
#define _FPCLASS_PD 5

inline double _copysign(double mag, double sgn) { return std::copysign(mag, sgn); }

// MSVC's _stat/struct _stat (distinct spelling from POSIX stat()/struct
// stat) - identical alias for both the type and function names works
// since both exist as POSIX stat()/struct stat.
#define _stat stat

// --- Unicode conversion (xrstring.h's UTF8_to_CP1251) --------------------
// Real iconv()-backed implementation, not a stub - this is genuine
// localization functionality (Cyrillic codepage conversion). Only the two
// specific encoding pairs this codebase's one call site actually uses
// (UTF-8<->UTF-16LE, UTF-16LE->CP1251) are implemented, not the full
// generality of the real Win32 API (arbitrary codepages/flags).
#define CP_UTF8 65001
#define CP_ACP 0

inline int MultiByteToWideChar(unsigned codePage, unsigned long /*flags*/, const char* src, int srcLen,
                                char16_t* dst, int dstCapacity)
{
	const char* fromEnc = (codePage == CP_UTF8) ? "UTF-8" : "";
	if (!fromEnc[0])
		return 0;

	iconv_t cd = iconv_open("UTF-16LE", fromEnc);
	if (cd == reinterpret_cast<iconv_t>(-1))
		return 0;

	size_t inBytesLeft = (srcLen < 0) ? std::strlen(src) : static_cast<size_t>(srcLen);
	char* inBuf = const_cast<char*>(src);

	if (dstCapacity == 0)
	{
		// Query mode: convert into a scratch buffer just to compute the
		// required length (iconv has no length-only query mode).
		std::vector<char> scratch(inBytesLeft * 4 + 8);
		char* outBuf = scratch.data();
		size_t outBytesLeft = scratch.size();
		size_t converted = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
		iconv_close(cd);
		if (converted == static_cast<size_t>(-1))
			return 0;
		return static_cast<int>((scratch.size() - outBytesLeft) / sizeof(char16_t));
	}

	char* outBuf = reinterpret_cast<char*>(dst);
	size_t outBytesLeft = static_cast<size_t>(dstCapacity) * sizeof(char16_t);
	size_t converted = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
	iconv_close(cd);
	if (converted == static_cast<size_t>(-1))
		return 0;
	return dstCapacity - static_cast<int>(outBytesLeft / sizeof(char16_t));
}

// --- Window/message-loop types (xrEngine's device.h/x_ray.h/xr_input.h/
// MonitorList.h header stubs - declarations-only pass, see
// playground/xray-monolith-vulkan-port-notes.md section 21) -------------
// Real window creation/message loop/DirectInput are a dedicated future
// SDL2/Vulkan bootstrap pass (out of scope here) - these just need to
// *exist* so the many otherwise-fully-portable files that pull in
// device.h transitively (stdafx.h includes it unconditionally) can
// compile. HWND/HMONITOR/HMODULE are opaque handles on Win32 anyway, so
// void* is a correct representation, not a placeholder.
using HWND = void*;
using HMONITOR = void*;
using HMODULE = void*;
using HINSTANCE = void*;
using LPSTR = char*;
using LPCSTR = const char*;
using UINT = unsigned int;
using UINT32 = unsigned int;
using LONG_PTR = intptr_t;
using WPARAM = UINT_PTR;
using LPARAM = LONG_PTR;
using LRESULT = LONG_PTR;

// _XR_RECT_DEFINED/_XR_POINT_DEFINED: project-invented guards (no real
// Windows SDK equivalent exists for plain RECT/POINT - unlike GUID_DEFINED
// below, which is a real SDK macro). xrRenderPC_R4 also pulls in
// dxvk-native's windows_base.h (via d3d9.h/d3d11.h), which independently
// defines RECT/POINT with identical layout; since this header is always
// included first in that build (transitively via xrEngine/stdafx.h, ahead
// of the D3D9/D3D11 headers), these defines let windows_base.h's copies
// detect that and skip themselves (see the matching guards added there)
// instead of hard-conflicting with these.
#define _XR_RECT_DEFINED
struct RECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};

#define _XR_POINT_DEFINED
struct POINT
{
	LONG x;
	LONG y;
};

// GDI object handles (Text_Console.h's CTextConsole - a GDI-rendered
// debug console for DEDICATED_SERVER builds; DEDICATED_SERVER is never
// defined in any real Monolith config, same dead-code-path category as
// _EDITOR/INGAME_EDITOR, see notes section 21a point 5 - but the class
// itself is still unconditionally declared, so these need to at least
// exist). Opaque handles on Win32 anyway, so void* is a correct
// representation, not a placeholder - same treatment as HWND/HMONITOR/
// HMODULE above.
using HDC = void*;
using HFONT = void*;
using HBRUSH = void*;
using HBITMAP = void*;

// TerminateProcess: this codebase's one call site (EventAPI.cpp's debug
// "quit" console command) just wants immediate, no-cleanup process exit -
// exactly POSIX _exit()'s contract, not a stub.
inline void TerminateProcess(void* /*process*/, unsigned exitCode)
{
	_exit(static_cast<int>(exitCode));
}

// _InterlockedCompareExchange: real MSVC <intrin.h> intrinsic (xr_object.cpp
// uses it for a lock-free "already processed this frame" check on
// CObject::dwFrame_AsCrow) - GCC/Clang's __sync_val_compare_and_swap
// builtin has identical semantics (full memory barrier, atomic CAS,
// returns the value that was at *dest before the exchange), just with
// comparand/exchange swapped in argument order.
inline long _InterlockedCompareExchange(volatile long* dest, long exchange, long comparand)
{
	return __sync_val_compare_and_swap(dest, comparand, exchange);
}

// _InterlockedExchange/_InterlockedDecrement: same MSVC <intrin.h> family
// as _InterlockedCompareExchange above (xrCPU_Pipe/ttapi.cpp's worker-
// thread-pool spinlock flags/queue-size counter) - GCC/Clang's
// __sync_lock_test_and_set/__sync_sub_and_fetch builtins have identical
// semantics (full memory barrier, atomic exchange/decrement, return the
// new/previous value per each function's real MSVC contract).
inline long _InterlockedExchange(volatile long* dest, long value)
{
	return __sync_lock_test_and_set(dest, value);
}
inline long _InterlockedDecrement(volatile long* dest)
{
	return __sync_sub_and_fetch(dest, 1);
}

// Keyboard-layout/toggle-key-state queries (line_edit_control.cpp's
// caps-lock/num-lock display and layout-switch hotkey) - real values (the
// documented winuser.h virtual-key codes / HKL_NEXT), but no live
// windowing layer to query yet in this pass (see notes section 21, Part
// 1 item 3 - xr_input.h is a declarations-only stub here too). Stubbed
// to always report "not toggled" / a no-op switch, same treatment as
// MessageBox/os_clipboard's documented gaps in xrCore - real behavior
// needs a future SDL2 keyboard-state query, not invented here.
using HKL = void*;
#define VK_CAPITAL 0x14
#define VK_NUMLOCK 0x90
#define HKL_NEXT 1
inline short GetKeyState(int /*vKey*/) { return 0; }
inline HKL ActivateKeyboardLayout(HKL /*hkl*/, unsigned /*flags*/) { return nullptr; }

// --- DirectInput (dinput.h) - portable stand-ins, declarations-only ----
// Real input goes through SDL2 once the window/input bootstrap is
// ported (see notes section 21a point 6) - these only need to exist so
// xr_input.h's CInput class declaration parses; no DirectInput call is
// ever actually made against them in this pass. Field layouts mirror the
// real dinput.h structs closely enough for sizeof()/member-access sites
// already in the codebase, but nothing here is wired to real HID input.
// GUID_DEFINED: real Windows SDK guard macro (guiddef.h), meant for exactly
// this situation - more than one header in the same translation unit
// providing a GUID definition. dxvk-native's windows_base.h already checks
// it (`#ifndef GUID_DEFINED`) before defining its own GUID, so defining it
// here after this struct is enough to make that copy a no-op instead of a
// conflicting redefinition. Data1 is uint32_t (not `unsigned long`, 8 bytes
// on Linux/LP64) to match the real Win32 GUID ABI - same LLP64 reasoning as
// LONG/ULONG above, and required for layout compatibility with
// windows_base.h's own (uint32_t Data1) definition.
#define GUID_DEFINED
struct GUID
{
	uint32_t       Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
};

struct DIDEVCAPS
{
	unsigned long dwSize;
	unsigned long dwFlags;
	unsigned long dwDevType;
	unsigned long dwAxes;
	unsigned long dwButtons;
	unsigned long dwPOVs;
	unsigned long dwFFSamplePeriod;
	unsigned long dwFFMinTimeResolution;
	unsigned long dwFirmwareRevision;
	unsigned long dwHardwareRevision;
	unsigned long dwFFDriverVersion;
};

struct DIDEVICEINSTANCE
{
	unsigned long dwSize;
	GUID guidInstance;
	GUID guidProduct;
	unsigned long dwDevType;
	char tszInstanceName[260];
	char tszProductName[260];
	GUID guidFFDriver;
	unsigned short wUsagePage;
	unsigned short wUsage;
};

struct DIDEVICEOBJECTINSTANCE
{
	unsigned long dwSize;
	GUID guidType;
	unsigned long dwOfs;
	unsigned long dwType;
	unsigned long dwFlags;
	char tszName[260];
	unsigned long dwFFMaxForce;
	unsigned long dwFFForceResolution;
	unsigned short wCollectionNumber;
	unsigned short wDesignatorIndex;
	unsigned short wUsagePage;
	unsigned short wUsage;
	unsigned long dwDimension;
	unsigned short wExponent;
	unsigned short wReserved;
};

struct DIDATAFORMAT
{
	unsigned long dwSize;
	unsigned long dwObjSize;
	unsigned long dwFlags;
	unsigned long dwDataSize;
	unsigned long dwNumObjs;
	void* rgodf;
};

struct IDirectInputDevice8 { virtual ~IDirectInputDevice8() {} };
struct IDirectInput8 { virtual ~IDirectInput8() {} };
using LPDIRECTINPUT8 = IDirectInput8*;
using LPDIRECTINPUTDEVICE8 = IDirectInputDevice8*;

// DIK_* scancodes - real, well-documented dinput.h values (the classic
// PC/AT keyboard scan-code set DirectInput exposes verbatim), not
// invented placeholders - kept numerically accurate in case any future
// SDL2 input pass wants to map real scancodes onto these.
#define DIK_ESCAPE          0x01
#define DIK_1                0x02
#define DIK_2                0x03
#define DIK_3                0x04
#define DIK_4                0x05
#define DIK_5                0x06
#define DIK_6                0x07
#define DIK_7                0x08
#define DIK_8                0x09
#define DIK_9                0x0A
#define DIK_0                0x0B
#define DIK_MINUS            0x0C
#define DIK_EQUALS           0x0D
#define DIK_BACK             0x0E
#define DIK_TAB              0x0F
#define DIK_Q                0x10
#define DIK_W                0x11
#define DIK_E                0x12
#define DIK_R                0x13
#define DIK_T                0x14
#define DIK_Y                0x15
#define DIK_U                0x16
#define DIK_I                0x17
#define DIK_O                0x18
#define DIK_P                0x19
#define DIK_LBRACKET         0x1A
#define DIK_RBRACKET         0x1B
#define DIK_RETURN           0x1C
#define DIK_LCONTROL         0x1D
#define DIK_A                0x1E
#define DIK_S                0x1F
#define DIK_D                0x20
#define DIK_F                0x21
#define DIK_G                0x22
#define DIK_H                0x23
#define DIK_J                0x24
#define DIK_K                0x25
#define DIK_L                0x26
#define DIK_SEMICOLON        0x27
#define DIK_APOSTROPHE       0x28
#define DIK_GRAVE            0x29
#define DIK_LSHIFT           0x2A
#define DIK_BACKSLASH        0x2B
#define DIK_Z                0x2C
#define DIK_X                0x2D
#define DIK_C                0x2E
#define DIK_V                0x2F
#define DIK_B                0x30
#define DIK_N                0x31
#define DIK_M                0x32
#define DIK_COMMA            0x33
#define DIK_PERIOD           0x34
#define DIK_SLASH            0x35
#define DIK_RSHIFT           0x36
#define DIK_MULTIPLY         0x37
#define DIK_LMENU            0x38
#define DIK_SPACE            0x39
#define DIK_CAPITAL          0x3A
#define DIK_F1               0x3B
#define DIK_F2               0x3C
#define DIK_F3               0x3D
#define DIK_F4               0x3E
#define DIK_F5               0x3F
#define DIK_F6               0x40
#define DIK_F7               0x41
#define DIK_F8               0x42
#define DIK_F9               0x43
#define DIK_F10              0x44
#define DIK_NUMLOCK          0x45
#define DIK_SCROLL           0x46
#define DIK_NUMPAD7          0x47
#define DIK_NUMPAD8          0x48
#define DIK_NUMPAD9          0x49
#define DIK_SUBTRACT         0x4A
#define DIK_NUMPAD4          0x4B
#define DIK_NUMPAD5          0x4C
#define DIK_NUMPAD6          0x4D
#define DIK_ADD              0x4E
#define DIK_NUMPAD1          0x4F
#define DIK_NUMPAD2          0x50
#define DIK_NUMPAD3          0x51
#define DIK_NUMPAD0          0x52
#define DIK_DECIMAL          0x53
#define DIK_OEM_102          0x56
#define DIK_F11              0x57
#define DIK_F12              0x58
#define DIK_NUMPADEQUALS     0x8D
#define DIK_NUMPADENTER      0x9C
#define DIK_RCONTROL         0x9D
#define DIK_NUMPADCOMMA      0xB3
#define DIK_DIVIDE           0xB5
#define DIK_SYSRQ            0xB7
#define DIK_RMENU            0xB8
#define DIK_PAUSE            0xC5
#define DIK_HOME             0xC7
#define DIK_UP               0xC8
#define DIK_PRIOR            0xC9
#define DIK_LEFT             0xCB
#define DIK_RIGHT            0xCD
#define DIK_END              0xCF
#define DIK_DOWN             0xD0
#define DIK_NEXT             0xD1
#define DIK_INSERT           0xD2
#define DIK_DELETE           0xD3
#define DIK_LWIN             0xDB
#define DIK_RWIN             0xDC
#define DIK_APPS             0xDD
#define DIK_UNLABELED        0x97
// Real, well-documented dinput.h scancode values (Japanese-keyboard/
// extra-key codes, same DIK_* family as the rest of this block) - first
// real caller: xrGame/xr_level_controller.cpp's DIK-name lookup table.
#define DIK_KANA             0x70
#define DIK_CONVERT          0x79
#define DIK_NOCONVERT        0x7B
#define DIK_YEN              0x7D
#define DIK_CIRCUMFLEX       0x90
#define DIK_AT               0x91
#define DIK_COLON            0x92
#define DIK_UNDERLINE        0x93
#define DIK_KANJI            0x94
#define DIK_STOP             0x95
#define DIK_AX               0x96
#define DIK_F13              0x64
#define DIK_F14              0x65
#define DIK_F15              0x66

// DIMOFS_* - real dinput.h DIMOUSESTATE field-byte-offset constants
// (used by Xr_input.cpp purely as small distinguishing ids passed through
// IInputReceiver::IR_OnMouseStop(int axis, int)'s first parameter, not as
// real struct offsets into anything on this port - same "kept numerically
// accurate in case a future real DirectInput-shaped consumer needs them"
// reasoning as the DIK_* table above).
#define DIMOFS_X 0
#define DIMOFS_Y 4
#define DIMOFS_Z 8

// Alternate names (dinput.h's own aliases, not this port's invention).
#define DIK_BACKSPACE        DIK_BACK
#define DIK_NUMPADSTAR       DIK_MULTIPLY
#define DIK_LALT             DIK_LMENU
#define DIK_CAPSLOCK         DIK_CAPITAL
#define DIK_NUMPADMINUS      DIK_SUBTRACT
#define DIK_NUMPADPLUS       DIK_ADD
#define DIK_NUMPADPERIOD     DIK_DECIMAL
#define DIK_NUMPADSLASH      DIK_DIVIDE
#define DIK_RALT             DIK_RMENU
#define DIK_UPARROW          DIK_UP
#define DIK_PGUP             DIK_PRIOR
#define DIK_LEFTARROW        DIK_LEFT
#define DIK_RIGHTARROW       DIK_RIGHT
#define DIK_DOWNARROW        DIK_DOWN
#define DIK_PGDN             DIK_NEXT

// WM_USER - real Win32 value (0x0400), needed only as a base integer
// constant for enum arithmetic (script_debugger_messages.h's dbg_messages
// enum); the mailslot-based script debugger IPC it belongs to is itself
// Windows-only and not ported, but this file is still reached transitively
// by other, real console-command registration code.
#define WM_USER 0x0400

inline int WideCharToMultiByte(unsigned codePage, unsigned long /*flags*/, const char16_t* src, int srcLen,
                                char* dst, int dstCapacity, const char* /*defaultChar*/, int* /*usedDefaultChar*/)
{
	// windows-1251 is CP1251's real iconv name; only codepage this
	// codebase's one call site ever passes.
	const char* toEnc = (codePage == 1251) ? "WINDOWS-1251" : "";
	if (!toEnc[0])
		return 0;

	iconv_t cd = iconv_open(toEnc, "UTF-16LE");
	if (cd == reinterpret_cast<iconv_t>(-1))
		return 0;

	size_t inBytesLeft = ((srcLen < 0) ? std::char_traits<char16_t>::length(src) : static_cast<size_t>(srcLen)) * sizeof(char16_t);
	char* inBuf = reinterpret_cast<char*>(const_cast<char16_t*>(src));
	char* outBuf = dst;
	size_t outBytesLeft = static_cast<size_t>(dstCapacity);

	size_t converted = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
	iconv_close(cd);
	if (converted == static_cast<size_t>(-1))
		return 0;
	return dstCapacity - static_cast<int>(outBytesLeft);
}
