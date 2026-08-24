#include "stdafx.h"
#include <errno.h>
#include <malloc.h>

#ifdef _WIN32

XRCORE_API void vminfo(size_t* _free, size_t* reserved, size_t* committed)
{
	MEMORY_BASIC_INFORMATION memory_info;
	memory_info.BaseAddress = 0;
	*_free = *reserved = *committed = 0;
	while (VirtualQuery(memory_info.BaseAddress, &memory_info, sizeof(memory_info)))
	{
		switch (memory_info.State)
		{
		case MEM_FREE:
			*_free += memory_info.RegionSize;
			break;
		case MEM_RESERVE:
			*reserved += memory_info.RegionSize;
			break;
		case MEM_COMMIT:
			*committed += memory_info.RegionSize;
			break;
		}
		memory_info.BaseAddress = (char*)memory_info.BaseAddress + memory_info.RegionSize;
	}
}

#else // !_WIN32

// Windows' VirtualQuery-based free/reserved/committed split is a Windows-
// specific memory model concept (address space can be "reserved" without
// being "committed" to physical/swap backing) that Linux's overcommit
// model doesn't have an equivalent three-way state for. Best-effort
// mapping from /proc/self/status for this diagnostic-only function:
// VmSize (total virtual address space) as "reserved", VmRSS (actually
// resident pages) as "committed", "free" left at 0 (no Linux equivalent
// of "reserved-but-unbacked" to report).
XRCORE_API void vminfo(size_t* _free, size_t* reserved, size_t* committed)
{
	*_free = *reserved = *committed = 0;
	FILE* f = fopen("/proc/self/status", "r");
	if (!f)
		return;
	char line[256];
	while (fgets(line, sizeof(line), f))
	{
		unsigned long kb = 0;
		if (sscanf(line, "VmSize: %lu kB", &kb) == 1)
			*reserved = static_cast<size_t>(kb) * 1024;
		else if (sscanf(line, "VmRSS: %lu kB", &kb) == 1)
			*committed = static_cast<size_t>(kb) * 1024;
	}
	fclose(f);
}

#endif // _WIN32

XRCORE_API void log_vminfo()
{
	size_t w_free, w_reserved, w_committed;
	vminfo(&w_free, &w_reserved, &w_committed);
	Msg(
		"* [win32]: free[%lld K], reserved[%lld K], committed[%lld K]",
		w_free / 1024,
		w_reserved / 1024,
		w_committed / 1024
	);
}

size_t xrMemory::mem_usage()
{
#ifdef _WIN32
	_HEAPINFO hinfo = {};
	int status;
	size_t bytesUsed = 0;
	while ((status = _heapwalk(&hinfo)) == _HEAPOK)
	{
		if (hinfo._useflag == _USEDENTRY)
			bytesUsed += hinfo._size;
	}
	switch (status)
	{
	case _HEAPEMPTY:
		break;
	case _HEAPEND:
		break;
	case _HEAPBADPTR:
		FATAL("bad pointer to heap");
		break;
	case _HEAPBADBEGIN:
		FATAL("bad start of heap");
		break;
	case _HEAPBADNODE:
		FATAL("bad node in heap");
		break;
	}
	return bytesUsed;
#else
	// MSVC's _heapwalk iterates individual heap blocks to sum used bytes;
	// glibc has no equivalent per-block walk API, but mallinfo2() gives
	// the same aggregate total directly (uordblks = bytes in use by
	// allocated chunks) without needing one. mallinfo2 (not the older,
	// deprecated mallinfo) specifically because mallinfo's fields are
	// `int` and silently overflow/wrap on multi-GB heaps, which a game
	// engine can easily reach.
	struct mallinfo2 mi = mallinfo2();
	return static_cast<size_t>(mi.uordblks);
#endif
}
