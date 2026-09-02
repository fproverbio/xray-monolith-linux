#pragma once

// Win32 file/file-mapping API subset used by this codebase's VFS layer
// (CreateFile/CreateFileMapping/MapViewOfFile/.../ReadFile/SetFilePointer),
// reimplemented on POSIX open()/mmap()/pread()/lseek(). Real functionality,
// not a stub - see playground/xray-monolith-vulkan-port-notes.md section 14.
//
// HANDLE is modeled as (fd + 1) packed into a pointer, so a 0/nullptr
// HANDLE never aliases a valid fd (fd 0 is stdin). CreateFileMapping
// dup()s the file descriptor rather than just returning it unchanged, so
// that closing both the "file" and "mapping" handles (as every call site
// in this codebase does) doesn't double-close the same fd - matches
// Win32's semantic of file handle and mapping handle being distinct
// objects. UnmapViewOfFile needs to know each mapping's length (Windows
// tracks that internally per-address; munmap() needs it explicit), so
// MapViewOfFile records address->length in a small table that
// UnmapViewOfFile consumes.

#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "posix_path_norm.h"

using HANDLE = void*;
#define INVALID_HANDLE_VALUE reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1))
#define INVALID_SET_FILE_POINTER 0xFFFFFFFFul

#define GENERIC_READ 0x1u
#define GENERIC_WRITE 0x2u
#define FILE_SHARE_READ 0x1u
#define FILE_SHARE_WRITE 0x2u
#define OPEN_EXISTING 3u
#define FILE_ATTRIBUTE_READONLY 0x1u
#define FILE_ATTRIBUTE_HIDDEN 0x2u
#define FILE_ATTRIBUTE_DIRECTORY 0x10u
#define FILE_ATTRIBUTE_NORMAL 0x80u

#define PAGE_READONLY 0x1u
#define PAGE_READWRITE 0x2u
#define FILE_MAP_READ 0x1u
#define FILE_MAP_WRITE 0x2u
#define FILE_MAP_ALL_ACCESS (FILE_MAP_READ | FILE_MAP_WRITE)

#define FILE_BEGIN 0u
#define FILE_CURRENT 1u
#define FILE_END 2u
#define FILE_FLAG_NO_BUFFERING 0x20000000u // ignored - CreateFile shim always goes through the page cache

inline int _xr_handle_to_fd(HANDLE h) { return static_cast<int>(reinterpret_cast<intptr_t>(h)) - 1; }
inline HANDLE _xr_fd_to_handle(int fd) { return reinterpret_cast<HANDLE>(static_cast<intptr_t>(fd) + 1); }

inline HANDLE CreateFile(const char* path, unsigned access, unsigned /*share*/, void* /*sec*/, unsigned creation,
                          unsigned /*flags*/, void* /*template*/)
{
	if (creation != OPEN_EXISTING)
		return INVALID_HANDLE_VALUE; // only path actually used in this codebase

	int oflags = 0;
	if ((access & GENERIC_READ) && (access & GENERIC_WRITE))
		oflags = O_RDWR;
	else if (access & GENERIC_WRITE)
		oflags = O_WRONLY;
	else
		oflags = O_RDONLY;

	int fd = open(xr_posix_path(path).c_str(), oflags);
	return (fd < 0) ? INVALID_HANDLE_VALUE : _xr_fd_to_handle(fd);
}

inline unsigned long GetFileSize(HANDLE h, unsigned long* highOut)
{
	if (highOut)
		*highOut = 0;
	struct stat st{};
	if (fstat(_xr_handle_to_fd(h), &st) != 0)
		return INVALID_SET_FILE_POINTER;
	return static_cast<unsigned long>(st.st_size);
}

inline HANDLE CreateFileMapping(HANDLE hFile, void* /*sec*/, unsigned /*protect*/, unsigned /*sizeHigh*/,
                                 unsigned /*sizeLow*/, const char* /*name*/)
{
	int dup_fd = dup(_xr_handle_to_fd(hFile));
	return (dup_fd < 0) ? INVALID_HANDLE_VALUE : _xr_fd_to_handle(dup_fd);
}

inline std::unordered_map<void*, size_t>& _xr_mapping_lengths()
{
	static std::unordered_map<void*, size_t> lengths;
	return lengths;
}

// CTextureDescrMngr::Load() spawns concurrent THM-loader threads that both
// funnel through CLocatorAPI::file_from_archive -> MapViewOfFile/
// UnmapViewOfFile, so this map (unlike the rest of this Win32-on-POSIX
// shim, which is only ever touched from a single thread at a time in
// practice) needs real locking rather than relying on caller discipline.
inline std::mutex& _xr_mapping_lengths_mutex()
{
	static std::mutex m;
	return m;
}

inline void* MapViewOfFile(HANDLE hMapping, unsigned access, unsigned offsetHigh, unsigned offsetLow,
                            size_t bytesToMap)
{
	int fd = _xr_handle_to_fd(hMapping);
	off_t offset = (static_cast<off_t>(offsetHigh) << 32) | static_cast<off_t>(offsetLow);
	size_t len = bytesToMap;
	if (len == 0)
	{
		struct stat st{};
		if (fstat(fd, &st) != 0)
			return nullptr;
		len = static_cast<size_t>(st.st_size) - static_cast<size_t>(offset);
	}
	int prot = (access & FILE_MAP_WRITE) ? (PROT_READ | PROT_WRITE) : PROT_READ;
	void* p = mmap(nullptr, len, prot, MAP_SHARED, fd, offset);
	if (p == MAP_FAILED)
		return nullptr;
	{
		std::lock_guard<std::mutex> lock(_xr_mapping_lengths_mutex());
		_xr_mapping_lengths()[p] = len;
	}
	return p;
}

inline bool UnmapViewOfFile(void* addr)
{
	size_t len = 0;
	{
		std::lock_guard<std::mutex> lock(_xr_mapping_lengths_mutex());
		auto& lengths = _xr_mapping_lengths();
		auto it = lengths.find(addr);
		if (it != lengths.end())
		{
			len = it->second;
			lengths.erase(it);
		}
	}
	return len ? (munmap(addr, len) == 0) : false;
}

inline bool CloseHandle(HANDLE h)
{
	return close(_xr_handle_to_fd(h)) == 0;
}

inline bool ReadFile(HANDLE h, void* buf, unsigned bytesToRead, unsigned* bytesRead, void* /*overlapped*/)
{
	ssize_t n = read(_xr_handle_to_fd(h), buf, bytesToRead);
	if (bytesRead)
		*bytesRead = (n < 0) ? 0 : static_cast<unsigned>(n);
	return n >= 0;
}

inline unsigned long SetFilePointer(HANDLE h, long distance, long* distanceHigh, unsigned method)
{
	int whence = (method == FILE_BEGIN) ? SEEK_SET : (method == FILE_CURRENT) ? SEEK_CUR : SEEK_END;
	off_t pos = lseek(_xr_handle_to_fd(h), distance, whence);
	if (pos < 0)
		return INVALID_SET_FILE_POINTER;
	if (distanceHigh)
		*distanceHigh = 0;
	return static_cast<unsigned long>(pos);
}

inline bool CopyFile(const char* src, const char* dst, bool failIfExists)
{
	std::string realSrc = xr_posix_path(src);
	std::string realDst = xr_posix_path(dst);
	if (failIfExists)
	{
		struct stat st{};
		if (stat(realDst.c_str(), &st) == 0)
			return false;
	}
	int in = open(realSrc.c_str(), O_RDONLY);
	if (in < 0)
		return false;
	int out = open(realDst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (out < 0)
	{
		close(in);
		return false;
	}
	char buf[65536];
	ssize_t n;
	bool ok = true;
	while ((n = read(in, buf, sizeof(buf))) > 0)
	{
		if (write(out, buf, static_cast<size_t>(n)) != n)
		{
			ok = false;
			break;
		}
	}
	if (n < 0)
		ok = false;
	close(in);
	close(out);
	return ok;
}

inline bool SetFileAttributes(const char* path, unsigned attrs)
{
	std::string realPath = xr_posix_path(path);
	struct stat st{};
	if (stat(realPath.c_str(), &st) != 0)
		return false;
	mode_t mode = st.st_mode;
	if (attrs & FILE_ATTRIBUTE_READONLY)
		mode &= ~static_cast<mode_t>(S_IWUSR | S_IWGRP | S_IWOTH);
	else
		mode |= S_IWUSR;
	return chmod(realPath.c_str(), mode) == 0;
}
