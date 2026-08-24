#pragma once

// MSVC CRT low-level I/O (<io.h>'s _open/_read/_write/_close/filelength
// family) mapped onto POSIX. See playground/xray-monolith-vulkan-port-notes.md
// section 14 (CMake skeleton bring-up).

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0 // no text/binary distinction on POSIX
#endif
#ifndef O_SEQUENTIAL
#define O_SEQUENTIAL 0 // no direct open() equivalent; posix_fadvise is the closest match, not needed for correctness
#endif
#ifndef _O_RDONLY
#define _O_RDONLY O_RDONLY
#endif
#ifndef _O_WRONLY
#define _O_WRONLY O_WRONLY
#endif
#ifndef _O_BINARY
#define _O_BINARY O_BINARY
#endif
#ifndef _O_CREAT
#define _O_CREAT O_CREAT
#endif
#ifndef _O_TRUNC
#define _O_TRUNC O_TRUNC
#endif
#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif
#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif
#ifndef _S_IREAD
#define _S_IREAD S_IRUSR
#endif

// Win32 sharing-mode flags for _sopen/_sopen_s - POSIX open() has no
// equivalent concept (advisory locking via flock()/fcntl() is a separate,
// opt-in mechanism), so these are accepted and ignored by the wrappers
// below rather than mapped to anything.
#define SH_DENYWR 0x20
#define _SH_DENYNO 0x40

inline int _sopen(const char* path, int flags, int /*shflag*/, int mode = 0666) { return open(path, flags, mode); }
inline errno_t _sopen_s(int* handle, const char* path, int flags, int /*shflag*/, int mode)
{
	*handle = open(path, flags, mode);
	return (*handle < 0) ? errno : 0;
}
inline FILE* _fdopen(int fd, const char* mode) { return fdopen(fd, mode); }
inline int _mkdir(const char* path) { return mkdir(path, 0777); }

// _sys_errlist[errno] (MSVC CRT global array) used as a printf %s arg
// throughout this codebase - strerror(errno) is the direct POSIX
// equivalent; this proxy lets `_sys_errlist[errno]`-shaped call sites
// keep their exact syntax.
struct _xr_sys_errlist_proxy
{
	const char* operator[](int err) const { return strerror(err); }
};
inline _xr_sys_errlist_proxy _sys_errlist;

inline unsigned long GetFileAttributes(const char* path)
{
	struct stat st{};
	if (stat(path, &st) != 0)
		return 0xFFFFFFFFul; // INVALID_FILE_ATTRIBUTES
	unsigned long attrs = 0;
	if (S_ISDIR(st.st_mode))
		attrs |= 0x10; // FILE_ATTRIBUTE_DIRECTORY
	if (!(st.st_mode & S_IWUSR))
		attrs |= FILE_ATTRIBUTE_READONLY;
	return attrs ? attrs : 0x80; // FILE_ATTRIBUTE_NORMAL
}

inline void CopyMemory(void* dst, const void* src, size_t n) { memcpy(dst, src, n); }

inline int _open(const char* path, int flags, int mode = 0666) { return open(path, flags, mode); }
inline int _read(int fd, void* buf, unsigned count) { return static_cast<int>(read(fd, buf, count)); }
inline int _write(int fd, const void* buf, unsigned count) { return static_cast<int>(write(fd, buf, count)); }
inline int _close(int fd) { return close(fd); }

inline long filelength(int fd)
{
	struct stat st{};
	if (fstat(fd, &st) != 0)
		return -1;
	return static_cast<long>(st.st_size);
}
inline long _filelength(int fd) { return filelength(fd); }
