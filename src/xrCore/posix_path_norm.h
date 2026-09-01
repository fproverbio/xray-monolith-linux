#pragma once

// The engine's internal virtual file system (LocatorAPI.cpp /
// LocatorAPI_defs.cpp) builds every path as a backslash-separated,
// force-lowercased string - that's baked deeply into its logic (directory
// vs. file entries in the file table are told apart by a trailing '\',
// and every lookup key is lowercased before comparison) and isn't
// something this port changes. On Windows that's harmless: NTFS treats
// '\' and '/' interchangeably and resolves names case-insensitively, so
// the lowercased, backslash-joined string still opens the right file. On
// a native Linux filesystem neither is true - '\' is an ordinary filename
// character, not a separator, and lookups are case-sensitive - so any of
// these engine-built paths reaching a real syscall unmodified either
// resolves to nothing or (worse) to an unrelated same-named-but-wrong-case
// entry. Every POSIX shim that receives one of these paths on its way to
// open()/opendir()/stat()/etc. must run it through xr_posix_path() first.
// See playground/xray-monolith-vulkan-port-notes.md section 14/15 for the
// bug this fixes (system.ltx resolving to a lowercased, backslash-joined
// path that doesn't exist on disk even in an install that works fine
// under Wine).

#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unordered_map>

inline std::string xr_posix_to_lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

// dir (already resolved to real casing) -> {lowercased entry name -> real entry name}.
// Never invalidated: only consulted on a stat() miss for the exact-case
// path, and the engine never renames files out from under itself at
// runtime, so a directory's casing map can't go stale mid-session.
inline std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& xr_ci_dir_cache()
{
	static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> cache;
	return cache;
}

inline const std::unordered_map<std::string, std::string>* xr_ci_list_dir(const std::string& dir)
{
	auto& cache = xr_ci_dir_cache();
	auto it = cache.find(dir);
	if (it != cache.end())
		return &it->second;

	DIR* d = opendir(dir.empty() ? "." : dir.c_str());
	if (!d)
		return nullptr;

	auto& entries = cache[dir];
	struct dirent* de;
	while ((de = readdir(d)) != nullptr)
		entries.emplace(xr_posix_to_lower(de->d_name), de->d_name);
	closedir(d);
	return &entries;
}

// Walks a '/'-separated path component by component, substituting in the
// real on-disk casing wherever the given casing doesn't exist exactly.
// Falls back to the given component unchanged wherever no case-insensitive
// match is found either (including a component that genuinely doesn't
// exist), so a truly-missing file still fails as "not found" rather than
// silently resolving somewhere unexpected.
inline std::string xr_resolve_case(const std::string& path)
{
	if (path.empty())
		return path;

	bool absolute = path.front() == '/';
	std::string resolved = absolute ? "/" : "";
	size_t start = absolute ? 1 : 0;

	while (start <= path.size())
	{
		size_t slash = path.find('/', start);
		size_t len = (slash == std::string::npos) ? std::string::npos : slash - start;
		std::string component = path.substr(start, len);

		if (!component.empty() && component != "." && component != "..")
		{
			std::string probe = resolved + component;
			struct stat st{};
			if (stat(probe.c_str(), &st) != 0)
			{
				const auto* entries = xr_ci_list_dir(resolved.empty() ? "." : resolved);
				if (entries)
				{
					auto found = entries->find(xr_posix_to_lower(component));
					if (found != entries->end())
						component = found->second;
				}
			}
		}

		resolved += component;
		if (slash == std::string::npos)
			break;
		resolved += "/";
		start = slash + 1;
	}

	return resolved;
}

// Single entry point for every POSIX shim: backslash -> slash, then
// case-insensitive component resolution against the real filesystem.
inline std::string xr_posix_path(const char* path)
{
	std::string s(path);
	std::replace(s.begin(), s.end(), '\\', '/');
	return xr_resolve_case(s);
}
