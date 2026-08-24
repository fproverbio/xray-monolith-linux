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
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iconv.h>
#include <malloc.h>
#include <map>
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
using WORD = unsigned short;
using DWORD = unsigned int;
#define CALLBACK // __stdcall calling-convention marker, no-op on x86-64 (single calling convention)
// DllMain() reason-for-call codes. Only ever reach a switch() body that's
// unreachable dead code on a static (non-DLL) build, but the constants
// themselves still need to exist for such code to compile at all.
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3
#define DLL_PROCESS_DETACH 0
using LONG = long;
using ULONG = unsigned long;
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
using PSTR = char*;
using LPVOID = void*;

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

// Atomic exchange, returns the previous value - GCC/Clang's
// __sync_lock_test_and_set builtin has identical semantics for this
// exact use (a spinlock test-and-set), so no need to migrate the call
// site to std::atomic.
inline long InterlockedExchange(volatile long* target, long value)
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
inline int stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
inline long long _atoi64(const char* s) { return std::atoll(s); }
inline unsigned long long _strtoui64(const char* s, char** end, int base) { return std::strtoull(s, end, base); }
inline int _vsnprintf(char* buf, size_t n, const char* fmt, va_list args) { return vsnprintf(buf, n, fmt, args); }
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
