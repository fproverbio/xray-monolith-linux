////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_manager_fixed.h
//	Created 	: 21.03.2002
//  Modified 	: 01.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Fixed vertex manager
////////////////////////////////////////////////////////////////////////////

#pragma once

// Template parameters renamed from `_path_id_type`/`_index_type` to
// `_TPathIdType`/`_TIndexType` - same self-shadowing-typedef bug class as
// vertex_manager_hash_fixed.h in this same batch (notes section 30b).
template <
	typename _TPathIdType,
	typename _TIndexType,
	u8 mask
>
struct CVertexManagerFixed
{
	template <template <typename _T> class T1>
	struct VertexManager
	{
		template <typename T2>
		struct _vertex : public T1<T2>
		{
			typedef _TIndexType _index_type;
			_index_type _index : 8 * sizeof(_index_type) - mask;
			_index_type _opened : mask;

			IC _index_type index() const
			{
				return (_index);
			}

			IC _index_type opened() const
			{
				return (_opened);
			}
		};
	};

	// `_data_storage`'s original default `= CBuilderAllocatorConstructor`
	// (bare, unspecialized class template used where a type is required)
	// is ill-formed and GCC diagnoses it eagerly as soon as this class is
	// instantiated, even though every real call site (via the
	// CFixedVertexManager macro in vertex_manager_fixed_inline.h) always
	// supplies `_data_storage` explicitly - confirmed via exhaustive grep,
	// the default's actual VALUE is genuinely never relied upon. Can't
	// just drop the default though: `_vertex`/`_index_vertex` (the two
	// preceding parameters) both have defaults, and C++ requires every
	// template parameter after the first defaulted one to also have a
	// default. `void` is a syntactically valid type that satisfies this
	// rule and is never actually substituted into `_data_storage::
	// template CDataStorage<...>` below since no real caller omits this
	// argument - default template arguments are only substituted/checked
	// when actually used.
	template <
		template <typename _T> class _vertex = CEmptyClassTemplate,
		template <typename _T1, typename _T2> class _index_vertex = CEmptyClassTemplate2,
		typename _data_storage = void
	>
	class CDataStorage : public _data_storage::template CDataStorage<VertexManager<_vertex>::template _vertex>
	{
	public:
		typedef typename _data_storage::template CDataStorage<
			VertexManager<_vertex>::template _vertex
		> inherited;
		typedef typename inherited::CGraphVertex CGraphVertex;
		typedef typename CGraphVertex::_index_type _index_type;

#pragma pack(push,1)
		template <typename _path_id_type>
		struct SGraphIndexVertex : public _index_vertex<CGraphVertex, SGraphIndexVertex<_path_id_type>>
		{
			_path_id_type m_path_id;
			CGraphVertex* m_vertex;
		};
#pragma pack(pop)

		typedef _TPathIdType _path_id_type;
		typedef SGraphIndexVertex<_path_id_type> CGraphIndexVertex;

	protected:
		_path_id_type m_current_path_id;
		u32 m_max_node_count;
		CGraphIndexVertex* m_indexes;

	public:
		IC CDataStorage(const u32 vertex_count);
		virtual ~CDataStorage();
		IC void init();
		IC bool is_opened(const CGraphVertex& vertex) const;
		IC bool is_visited(const _index_type& vertex_id) const;
		IC bool is_closed(const CGraphVertex& vertex) const;
		IC CGraphVertex& get_node(const _index_type& vertex_id) const;
		IC CGraphVertex& create_vertex(CGraphVertex& vertex, const _index_type& vertex_id);
		IC void add_opened(CGraphVertex& vertex);
		IC void add_closed(CGraphVertex& vertex);
		IC _path_id_type current_path_id() const;
	};
};

#include "vertex_manager_fixed_inline.h"
