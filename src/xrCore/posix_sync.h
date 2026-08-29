#pragma once

// Win32 synchronization/timing/system-info primitives used by this
// codebase, reimplemented on POSIX pthreads/clock_gettime/sysconf. Real
// functionality, not stubs - see
// playground/xray-monolith-vulkan-port-notes.md section 14.

#include <cerrno>
#include <cstdint>
#include <ctime>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <vector>

// BOOL/TRUE/FALSE now defined in win32_compat.h (included before this
// file) - moved there because win32_compat.h's own functions started
// needing BOOL too, and it's the first-loaded of the compat headers.

// --- CRITICAL_SECTION (used via xrCriticalSection, xrSyncronize.cpp) ---
// Win32 critical sections are recursive by default - match that.
struct CRITICAL_SECTION
{
	pthread_mutex_t mutex;
};

inline void InitializeCriticalSection(CRITICAL_SECTION* cs)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&cs->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(&cs->mutex); }
inline void EnterCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_lock(&cs->mutex); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_unlock(&cs->mutex); }
inline BOOL TryEnterCriticalSection(CRITICAL_SECTION* cs) { return pthread_mutex_trylock(&cs->mutex) == 0; }

// --- SRWLOCK (used via xrSRWLock, xrSyncronize.cpp) ---------------------
struct SRWLOCK
{
	pthread_rwlock_t lock;
};

inline void InitializeSRWLock(SRWLOCK* l) { pthread_rwlock_init(&l->lock, nullptr); }
inline void AcquireSRWLockExclusive(SRWLOCK* l) { pthread_rwlock_wrlock(&l->lock); }
inline void ReleaseSRWLockExclusive(SRWLOCK* l) { pthread_rwlock_unlock(&l->lock); }
inline void AcquireSRWLockShared(SRWLOCK* l) { pthread_rwlock_rdlock(&l->lock); }
inline void ReleaseSRWLockShared(SRWLOCK* l) { pthread_rwlock_unlock(&l->lock); }
inline BOOL TryAcquireSRWLockExclusive(SRWLOCK* l) { return pthread_rwlock_trywrlock(&l->lock) == 0; }
inline BOOL TryAcquireSRWLockShared(SRWLOCK* l) { return pthread_rwlock_tryrdlock(&l->lock) == 0; }

// --- Named mutex + wait API (LocatorAPI_Notifications.cpp) --------------
// Only ever used process-locally in this codebase (not cross-process), so
// only ever used in LocatorAPI_Notifications.cpp, and only as a one-shot
// termination signal (created already-owned by the creating thread,
// worker thread blocks on it, creating thread Releases it once at
// shutdown to wake the worker and let it exit) - not real mutual-
// exclusion. That usage is exactly an eventfd's semantics (starts
// unsignaled, one write latches it signaled and readable-forever in
// level-triggered mode), and representing it as a real fd - like the
// FindFirstChangeNotification family below - lets WaitForMultipleObjects
// be a real poll() across everything uniformly instead of needing to
// distinguish handle "kinds".
#define INFINITE 0xFFFFFFFFu
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 258u
#define WAIT_FAILED 0xFFFFFFFFu

inline HANDLE CreateMutex(void* /*sec*/, BOOL /*initialOwner*/, const char* /*name*/)
{
	int fd = eventfd(0, EFD_CLOEXEC);
	return (fd < 0) ? INVALID_HANDLE_VALUE : _xr_fd_to_handle(fd);
}
inline BOOL ReleaseMutex(HANDLE h)
{
	uint64_t one = 1;
	return write(_xr_handle_to_fd(h), &one, sizeof(one)) == sizeof(one);
}
inline unsigned long WaitForSingleObject(HANDLE h, unsigned long ms)
{
	struct pollfd pfd{_xr_handle_to_fd(h), POLLIN, 0};
	int r = poll(&pfd, 1, (ms == INFINITE) ? -1 : static_cast<int>(ms));
	return (r > 0) ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}

// --- Manual-reset event (Level.cpp's spawn-antifreeze prefetch signal) --
// Same eventfd-as-HANDLE representation as CreateMutex above; regular
// (non-EFD_SEMAPHORE) eventfd semantics already give exactly manual-reset
// behavior for WaitForSingleObject's poll()-only wait (it never reads the
// fd, so a write leaves it readable-forever until something explicitly
// drains it) - only auto-reset events would need WaitForSingleObject
// itself to drain on a successful wait, and nothing in this codebase
// creates one, so that case isn't implemented.
inline HANDLE CreateEvent(void* /*sec*/, BOOL /*bManualReset*/, BOOL bInitialState, const char* /*name*/)
{
	int fd = eventfd(bInitialState ? 1 : 0, EFD_CLOEXEC | EFD_NONBLOCK);
	return (fd < 0) ? INVALID_HANDLE_VALUE : _xr_fd_to_handle(fd);
}
inline BOOL SetEvent(HANDLE h)
{
	uint64_t one = 1;
	return write(_xr_handle_to_fd(h), &one, sizeof(one)) == sizeof(one);
}
inline BOOL ResetEvent(HANDLE h)
{
	uint64_t val;
	// Non-blocking: drains the counter back to 0 if it was set, and simply
	// returns EAGAIN (a normal, expected "already unsignaled") otherwise -
	// the fd was created with EFD_NONBLOCK specifically for this.
	ssize_t r = read(_xr_handle_to_fd(h), &val, sizeof(val));
	return (r == sizeof(val)) || (errno == EAGAIN);
}
inline unsigned long WaitForMultipleObjects(unsigned long count, const HANDLE* handles, BOOL waitAll, unsigned long ms)
{
	std::vector<struct pollfd> pfds(count);
	for (unsigned long i = 0; i < count; ++i)
		pfds[i] = {_xr_handle_to_fd(handles[i]), POLLIN, 0};

	int r = poll(pfds.data(), count, (ms == INFINITE) ? -1 : static_cast<int>(ms));
	if (r <= 0)
		return (r == 0) ? WAIT_TIMEOUT : WAIT_FAILED;

	if (waitAll)
		return WAIT_OBJECT_0; // not needed by this codebase's one call site (always false)

	for (unsigned long i = 0; i < count; ++i)
		if (pfds[i].revents & POLLIN)
			return WAIT_OBJECT_0 + i;
	return WAIT_FAILED;
}

// --- FindFirstChangeNotification family (LocatorAPI_Notifications.cpp) ---
// Real inotify-backed implementation. Architecturally different from
// Win32's model (one HANDLE per watched directory vs. inotify's one fd
// covering many watches) - rather than touch the calling code, each
// "notification handle" here gets its own private inotify instance with
// a single watch, so it stays a real, poll()-able fd and slots directly
// into WaitForMultipleObjects above without the caller needing to know
// the difference.
#define FILE_NOTIFY_CHANGE_FILE_NAME 0x1u
#define FILE_NOTIFY_CHANGE_DIR_NAME 0x2u
#define FILE_NOTIFY_CHANGE_LAST_WRITE 0x10u

inline HANDLE FindFirstChangeNotification(const char* path, BOOL watchSubtree, unsigned filter)
{
	int ifd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if (ifd < 0)
		return INVALID_HANDLE_VALUE;

	uint32_t mask = IN_ONLYDIR;
	if (filter & (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME))
		mask |= IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO;
	if (filter & FILE_NOTIFY_CHANGE_LAST_WRITE)
		mask |= IN_MODIFY | IN_CLOSE_WRITE;

	// inotify has no native recursive-watch flag (watchSubtree); this
	// codebase's one caller (RegisterPath) passes bRecurse through but
	// nothing downstream currently relies on subdirectory changes
	// specifically - a single top-level watch is what the original
	// FindFirstChangeNotification call effectively achieved too (Windows'
	// "watch subtree" is opt-in per call, and change events only carry a
	// directory-level granularity here regardless).
	(void)watchSubtree;

	if (inotify_add_watch(ifd, path, mask) < 0)
	{
		close(ifd);
		return INVALID_HANDLE_VALUE;
	}
	return _xr_fd_to_handle(ifd);
}

inline BOOL FindNextChangeNotification(HANDLE h)
{
	// Windows re-arms the notification automatically here; inotify's
	// watch stays armed continuously (it's event-driven, not one-shot),
	// so this just needs to drain already-queued events so the fd stops
	// appearing "readable" for changes the caller already processed.
	int fd = _xr_handle_to_fd(h);
	char buf[4096];
	while (read(fd, buf, sizeof(buf)) > 0) {}
	return TRUE;
}

inline BOOL FindCloseChangeNotification(HANDLE h) { return close(_xr_handle_to_fd(h)) == 0; }

// --- Misc ----------------------------------------------------------------
inline void Sleep(unsigned long ms)
{
	struct timespec ts{static_cast<time_t>(ms / 1000), static_cast<long>((ms % 1000) * 1000000L)};
	nanosleep(&ts, nullptr);
}

inline unsigned long GetLastError() { return static_cast<unsigned long>(errno); }

// IsBadReadPtr is inherently unsafe/deprecated even on Windows (Microsoft's
// own docs recommend against it); no portable equivalent exists without
// signal-handler trickery. Every call site in this codebase treats a
// nonzero return as "bad" - returning 0 ("assume it's fine") is the
// standard porting fallback other engines use for this exact function.
inline int IsBadReadPtr(const void*, size_t) { return 0; }

// --- QueryPerformanceCounter/Frequency (FTimer.h) ------------------------
using LARGE_INTEGER = int64_t;
using PLARGE_INTEGER = int64_t*;

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* freq)
{
	*freq = 1000000000LL; // clock_gettime is nanosecond-resolution
	return TRUE;
}
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* counter)
{
	struct timespec ts{};
	clock_gettime(CLOCK_MONOTONIC, &ts);
	*counter = static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
	return TRUE;
}

// --- GetSystemInfo (LocatorAPI.cpp ctor, page-size/allocation-granularity) ---
struct SYSTEM_INFO
{
	unsigned long dwPageSize;
	unsigned long dwAllocationGranularity;
};

inline void GetSystemInfo(SYSTEM_INFO* si)
{
	long page = sysconf(_SC_PAGESIZE);
	si->dwPageSize = static_cast<unsigned long>(page);
	// Windows' allocation granularity (typically 64KB) is a distinct,
	// coarser-grained concept than page size, for VirtualAlloc address
	// spacing; POSIX has no equivalent notion, page size is the closest
	// meaningful value here.
	si->dwAllocationGranularity = static_cast<unsigned long>(page);
}
