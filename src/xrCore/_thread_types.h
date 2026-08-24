#pragma once

#include <atomic>

// Atomic types
using xr_atomic_u32 = std::atomic_uint32_t;
using xr_atomic_s32 = std::atomic_int;
using xr_atomic_bool = std::atomic_bool;

#ifdef _MSC_VER

#include <ppl.h>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

// Tasks Redefinition
using xr_task_group = concurrency::task_group;

template <typename T, typename U>
using xr_concurrent_unordered_map = concurrency::concurrent_unordered_map<T, U>;

template <typename T, typename allocator = xalloc<T>>
using xr_concurrent_vector = concurrency::concurrent_vector<T, allocator>;

template<typename BlockRangeType, typename Body>
inline void xr_parallel_for(BlockRangeType Begin, BlockRangeType End, Body Functor)
{
	concurrency::parallel_for(Begin, End, Functor);
}

template<typename Index, typename Body>
inline void xr_parallel_foreach(Index Begin, Index End, Body Functor)
{
	concurrency::parallel_for_each(Begin, End, Functor);
}

// PPL behaviour - fallback to std::sort if chunk size < 2048 and cores < 2
template<typename Data, typename Body>
inline void xr_parallel_sort(Data& data, Body functor)
{
	concurrency::parallel_sort(std::begin(data), std::end(data), functor);
}

#else // !_MSC_VER

// PORT TODO (see playground/xray-monolith-vulkan-port-notes.md section 11):
// MSVC's PPL (concurrency::*) has no Linux equivalent; its API shape is
// close to oneTBB's (concurrency::task_group ~ tbb::task_group,
// concurrency::concurrent_vector ~ tbb::concurrent_vector, etc), and the
// TBB decision (link system libtbb-dev) has already been made for the
// unrelated tbb::parallel_for call sites elsewhere in the engine - once
// TBB is wired into the CMake build, swap the definitions below for real
// tbb::* equivalents. Every call site that uses xr_task_group/
// xr_parallel_for/xr_concurrent_vector/etc elsewhere in the engine stays
// unchanged either way - only this file needs to change.
//
// For now: minimal serial fallback using only the standard library, so
// the tree compiles without pulling in a new external dependency in the
// middle of CMake skeleton bring-up. Correct, just not parallel yet.

#include <algorithm>
#include <unordered_map>
#include <vector>

class xr_task_group
{
public:
	template <typename Func>
	void run(Func f) { f(); }
	void run_and_wait() {}
	void wait() {}
	void cancel() {}
};

template <typename T, typename U>
using xr_concurrent_unordered_map = std::unordered_map<T, U>;

template <typename T, typename allocator = xalloc<T>>
using xr_concurrent_vector = std::vector<T, allocator>;

template<typename BlockRangeType, typename Body>
inline void xr_parallel_for(BlockRangeType Begin, BlockRangeType End, Body Functor)
{
	for (auto it = Begin; it != End; ++it)
		Functor(it);
}

template<typename Index, typename Body>
inline void xr_parallel_foreach(Index Begin, Index End, Body Functor)
{
	std::for_each(Begin, End, Functor);
}

template<typename Data, typename Body>
inline void xr_parallel_sort(Data& data, Body functor)
{
	std::sort(std::begin(data), std::end(data), functor);
}

#endif // _MSC_VER

template<typename Data, typename Body>
inline void xr_sort(Data& data, Body functor)
{
	std::sort(std::begin(data), std::end(data), functor);
}

template<typename Data, typename Body>
inline void xr_stable_sort(Data& data, Body functor)
{
	std::stable_sort(std::begin(data), std::end(data), functor);
}
