#pragma once

// MSVC's <io.h> _findfirst/_findnext/_findclose + _finddata_t, reimplemented
// on top of POSIX opendir/readdir/stat. Field surface limited to what
// LocatorAPI.cpp actually reads (name, attrib & (_A_SUBDIR|_A_HIDDEN),
// size, time_write) - see playground/xray-monolith-vulkan-port-notes.md
// section 13 (CMake skeleton bring-up) for context. Intentionally not a
// full-fidelity port of the MS CRT struct.

#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>

#include "posix_path_norm.h"

#define _A_SUBDIR 0x10
#define _A_HIDDEN 0x02

struct _finddata_t
{
	char name[260];
	unsigned attrib;
	long size;
	long time_write;
};

struct _xrcore_find_state
{
	DIR* dir;
	std::string path;
};

inline bool _xrcore_fill_finddata(const std::string& full_path, const char* name, _finddata_t& out)
{
	struct stat st{};
	if (stat(full_path.c_str(), &st) != 0)
		return false;

	std::strncpy(out.name, name, sizeof(out.name) - 1);
	out.name[sizeof(out.name) - 1] = 0;

	out.attrib = S_ISDIR(st.st_mode) ? _A_SUBDIR : 0;
	if (name[0] == '.')
		out.attrib |= _A_HIDDEN;

	out.size = static_cast<long>(st.st_size);
	out.time_write = static_cast<long>(st.st_mtime);
	return true;
}

inline intptr_t _findfirst(const char* pattern, _finddata_t* out)
{
	// Monolith always passes "<dir>\*" or "<dir>/*" - take the directory
	// part, ignore the wildcard (only ever "*" in practice).
	std::string dir(pattern);
	size_t slash = dir.find_last_of("/\\");
	dir = (slash == std::string::npos) ? "." : dir.substr(0, slash);
	dir = xr_posix_path(dir.c_str());

	DIR* d = opendir(dir.c_str());
	if (!d)
		return -1;

	auto* state = new _xrcore_find_state{d, dir};

	struct dirent* de;
	while ((de = readdir(state->dir)) != nullptr)
	{
		std::string full = state->path + "/" + de->d_name;
		if (_xrcore_fill_finddata(full, de->d_name, *out))
			return reinterpret_cast<intptr_t>(state);
	}

	closedir(state->dir);
	delete state;
	return -1;
}

inline int _findnext(intptr_t handle, _finddata_t* out)
{
	auto* state = reinterpret_cast<_xrcore_find_state*>(handle);
	struct dirent* de;
	while ((de = readdir(state->dir)) != nullptr)
	{
		std::string full = state->path + "/" + de->d_name;
		if (_xrcore_fill_finddata(full, de->d_name, *out))
			return 0;
	}
	return -1;
}

inline int _findclose(intptr_t handle)
{
	auto* state = reinterpret_cast<_xrcore_find_state*>(handle);
	closedir(state->dir);
	delete state;
	return 0;
}
