////////////////////////////////////////////////////////////////////////////
//	Module 		: a_star.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Implementation of the A* (a-star) algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "vertex_path.h"
#include "data_storage_constructor.h"
#include "dijkstra.h"

namespace AStar
{
	// Same self-shadowing-typedef bug class as everywhere else in this
	// batch (notes section 30b) - `_Vertex`'s own outer template parameter
	// renamed to `_TDistType` so the nested `_vertex<T2>`'s `typedef
	// _dist_type _dist_type;` doesn't shadow it; exposed member-typedef
	// name kept as `_dist_type` since external code (CAStar's own base
	// class list a few lines below) never references it by qualified name.
	template <
		typename _TDistType,
		template <typename _T> class T1
	>
	struct _Vertex
	{
		template <typename T2>
		struct _vertex : public T1<T2>
		{
			typedef _TDistType _dist_type;

			_dist_type _g;
			_dist_type _h;

			IC _dist_type& g()
			{
				return (_g);
			}

			IC _dist_type& h()
			{
				return (_h);
			}
		};
	};
}

// Same nested template-template-parameter shadowing bug class/fix as
// dijkstra.h/data_storage_constructor.h/
// manager_builder_allocator_constructor.h in this same batch - every
// nesting level renamed to its own unique placeholder names.
// CAStar's own outer `_dist_type` template parameter renamed to
// `_TDistType` - it used to be re-typedef'd at class scope from
// `CGraphVertex::_dist_type` back to the same name `_dist_type`, which
// GCC flags as "shadows template parameter" (same self-shadowing bug
// class as elsewhere in this batch, notes section 30b) once actually
// instantiated. Exposed member-typedef name kept as `_dist_type`.
template <
	typename _TDistType,
	typename _priority_queue,
	typename _vertex_manager,
	typename _vertex_allocator,
	bool euclidian_heuristics = true,
	typename _data_storage_base = CVertexPath<euclidian_heuristics>,
	template <typename _as_vT> class _vertex = CEmptyClassTemplate,
	template <
		typename _as_bac1,
		typename _as_bac2
	>
	class _builder_allocator_constructor = CBuilderAllocatorConstructor,
	template <
		typename _as_mbac1,
		typename _as_mbac2,
		typename _as_mbac3,
		template <
			typename _as_mbac4a,
			typename _as_mbac4b
		>
		class _as_mbac4
	>
	class _manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor,
	template <
		typename _as_dsc_algorithm,
		typename _as_dsc_manager,
		typename _as_dsc_builder,
		typename _as_dsc_allocator,
		template <typename _as_dsc_vT> class _as_dsc_vertex,
		template <
			typename _as_dsc_bac1,
			typename _as_dsc_bac2
		>
		class _as_dsc_builder_allocator_constructor = CBuilderAllocatorConstructor,
		template <
			typename _as_dsc_mbac1,
			typename _as_dsc_mbac2,
			typename _as_dsc_mbac3,
			template <
				typename _as_dsc_mbac4a,
				typename _as_dsc_mbac4b
			>
			class _as_dsc_mbac4
		>
		class _as_dsc_manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor
	>
	class _data_storage_constructor = CDataStorageConstructor,
	typename _iteration_type = u32
>
class CAStar : public CDijkstra<
		_TDistType,
		_priority_queue,
		_vertex_manager,
		_vertex_allocator,
		euclidian_heuristics,
		_data_storage_base,
		AStar::_Vertex<_TDistType, _vertex>::template _vertex,
		_builder_allocator_constructor,
		_manager_builder_allocator_constructor,
		_data_storage_constructor,
		_iteration_type
	>
{
protected:
	typedef CDijkstra<
		_TDistType,
		_priority_queue,
		_vertex_manager,
		_vertex_allocator,
		euclidian_heuristics,
		_data_storage_base,
		AStar::_Vertex<_TDistType, _vertex>::template _vertex,
		_builder_allocator_constructor,
		_manager_builder_allocator_constructor,
		_data_storage_constructor,
		_iteration_type
	> inherited;
	typedef typename inherited::CGraphVertex CGraphVertex;
	typedef typename CGraphVertex::_dist_type _dist_type;
	typedef typename CGraphVertex::_index_type _index_type;

protected:
	template <typename _PathManager>
	IC void initialize(_PathManager& path_manager);
	template <typename _PathManager>
	IC bool step(_PathManager& path_manager);

public:
	IC CAStar(const u32 max_vertex_count);
	virtual ~CAStar();
	template <typename _PathManager>
	IC bool find(_PathManager& path_manager);
};

#include "a_star_inline.h"
