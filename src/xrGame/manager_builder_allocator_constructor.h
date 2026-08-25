////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_builder_allocator_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Manager builder allocator constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "builder_allocator_constructor.h"

// The template-template-parameter _builder_allocator_constructor's own
// parameter list originally reused the outer template's own _builder/
// _allocator names, shadowing them - MSVC accepts this as an extension,
// GCC hard-errors ("declaration of template parameter shadows template
// parameter"). Renamed to _builder_param/_allocator_param (pure signature
// placeholders, never referenced by name) - same bug class/fix as
// dijkstra.h/a_star.h/data_storage_constructor.h in this same batch.
template <
	typename _manager,
	typename _builder,
	typename _allocator,
	template <
		typename _builder_param,
		typename _allocator_param
	>
	class _builder_allocator_constructor = CBuilderAllocatorConstructor
>
struct CManagerBuilderAllocatorConstructor
{
	template <
		template <typename T> class _vertex = CEmptyClassTemplate,
		template <typename T1, typename T2> class _index_vertex = CEmptyClassTemplate2
	>
	class CDataStorage :
		public _manager::template CDataStorage<
			_vertex,
			_index_vertex,
			_builder_allocator_constructor<
				_builder,
				_allocator
			>
		>
	{
	public:
		typedef typename _manager::template CDataStorage<
			_vertex,
			_index_vertex,
			_builder_allocator_constructor<
				_builder,
				_allocator
			>
		> inherited;
		typedef typename inherited::inherited inherited_allocator;
		typedef typename inherited::CGraphVertex CGraphVertex;
		typedef typename CGraphVertex::_index_type _index_type;

	public:
		IC CDataStorage(const u32 vertex_count);
		virtual ~CDataStorage();
		IC void init();
		IC CGraphVertex& create_vertex(const _index_type& vertex_id);
	};
};

#include "manager_builder_allocator_constructor_inline.h"
