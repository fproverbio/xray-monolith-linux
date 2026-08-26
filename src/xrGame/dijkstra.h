////////////////////////////////////////////////////////////////////////////
//	Module 		: dijkstra.h
//	Created 	: 21.03.2002
//  Modified 	: 02.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Implementation of the Dijkstra algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "vertex_path.h"
#include "data_storage_constructor.h"

// Every nested template-template-parameter's own parameter list below was
// originally spelled with reused placeholder names (_1/_2/_3/_4/_T/
// _algorithm/_manager/_builder/_allocator/_vertex/
// _builder_allocator_constructor/_manager_builder_allocator_constructor)
// that shadow an enclosing template parameter of the exact same name -
// MSVC accepts this as an extension, GCC hard-errors ("declaration of
// template parameter shadows template parameter"). Renamed every nesting
// level to its own unique placeholder names (pure signature placeholders,
// never referenced anywhere by name) - same bug class/fix as
// data_storage_constructor.h/manager_builder_allocator_constructor.h in
// this same batch.
//
// CDijkstra's own outer `_dist_type` parameter renamed to `_TDistType` -
// separately, it was ALSO re-typedef'd right back to the same name twice
// (once in the nested `_Vertex<T1>` below, once at class scope from
// `CGraphVertex::_dist_type`) - the self-shadowing-typedef bug class
// (notes section 30b), not the template-template-parameter one the
// comment above describes. Exposed member-typedef name kept as
// `_dist_type` both places.
template <
	typename _TDistType,
	typename _priority_queue,
	typename _vertex_manager,
	typename _vertex_allocator,
	bool euclidian_heuristics = true,
	typename _data_storage_base = CVertexPath<euclidian_heuristics>,
	template <typename _dj_vT> class _vertex = CEmptyClassTemplate,
	template <
		typename _dj_bac1,
		typename _dj_bac2
	>
	class _builder_allocator_constructor = CBuilderAllocatorConstructor,
	template <
		typename _dj_mbac1,
		typename _dj_mbac2,
		typename _dj_mbac3,
		template <
			typename _dj_mbac4a,
			typename _dj_mbac4b
		>
		class _dj_mbac4
	>
	class _manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor,
	template <
		typename _dj_dsc_algorithm,
		typename _dj_dsc_manager,
		typename _dj_dsc_builder,
		typename _dj_dsc_allocator,
		template <typename _dj_dsc_vT> class _dj_dsc_vertex,
		template <
			typename _dj_dsc_bac1,
			typename _dj_dsc_bac2
		>
		class _dj_dsc_builder_allocator_constructor = CBuilderAllocatorConstructor,
		template <
			typename _dj_dsc_mbac1,
			typename _dj_dsc_mbac2,
			typename _dj_dsc_mbac3,
			template <
				typename _dj_dsc_mbac4a,
				typename _dj_dsc_mbac4b
			>
			class _dj_dsc_mbac4
		>
		class _dj_dsc_manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor
	>
	class _data_storage_constructor = CDataStorageConstructor,
	typename _iteration_type = u32
>
class CDijkstra
{
public:
	template <typename T1>
	struct _Vertex : public _vertex<T1>
	{
		typedef _TDistType _dist_type;

		_dist_type _f;
		T1* _back;

		IC _dist_type& f()
		{
			return (_f);
		}

		IC const _dist_type& f() const
		{
			return (_f);
		}

		IC T1*& back()
		{
			return (_back);
		}
	};


	typedef _data_storage_constructor<
		_priority_queue,
		_vertex_manager,
		_data_storage_base,
		_vertex_allocator,
		_Vertex,
		_builder_allocator_constructor,
		_manager_builder_allocator_constructor
	> CDataStorage;

protected:
	typedef typename CDataStorage::CGraphVertex CGraphVertex;
	typedef typename CGraphVertex::_dist_type _dist_type;
	typedef typename CGraphVertex::_index_type _index_type;

protected:
	bool m_search_started;
	CDataStorage* m_data_storage;

protected:
	template <typename _PathManager>
	IC void initialize(_PathManager& path_manager);
	template <typename _PathManager>
	IC bool step(_PathManager& path_manager);
	template <typename _PathManager>
	IC void finalize(_PathManager& path_manager);

public:
	IC CDijkstra(const u32 max_vertex_count);
	virtual ~CDijkstra();
	template <typename _PathManager>
	IC bool find(_PathManager& path_manager);
	IC CDataStorage& data_storage();
	IC const CDataStorage& data_storage() const;
};

#include "dijkstra_inline.h"
