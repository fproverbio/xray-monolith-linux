////////////////////////////////////////////////////////////////////////////
//	Module 		: data_storage_constructor.h
//	Created 	: 21.03.2002
//  Modified 	: 28.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Data storage constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>
class CEmptyClassTemplate
{
};

template <typename T1, typename T2>
class CEmptyClassTemplate2
{
};

#include "manager_builder_allocator_constructor.h"

// Every nested template-template-parameter's own parameter list below was
// originally spelled with reused placeholder names (_1/_2/_3/_4/_T) that
// shadow an enclosing template parameter of the exact same name - MSVC
// accepts this as an extension, GCC hard-errors ("declaration of template
// parameter shadows template parameter"). Renamed every nesting level to
// its own unique placeholder names (pure signature placeholders, never
// referenced anywhere by name) - same bug class/fix as
// manager_builder_allocator_constructor.h in this same batch.
template <
	typename _algorithm,
	typename _manager,
	typename _builder,
	typename _allocator,
	template <typename _dsc_vT> class _vertex = CEmptyClassTemplate,
	template <
		typename _dsc_bac1,
		typename _dsc_bac2
	>
	class _builder_allocator_constructor = CBuilderAllocatorConstructor,
	template <
		typename _dsc_mbac1,
		typename _dsc_mbac2,
		typename _dsc_mbac3,
		template <
			typename _dsc_mbac4a,
			typename _dsc_mbac4b
		>
		class _dsc_mbac4
	>
	class _manager_builder_allocator_constructor = CManagerBuilderAllocatorConstructor
>
struct CDataStorageConstructor :
	public _algorithm::template CDataStorage<
		_manager_builder_allocator_constructor<
			_manager,
			_builder,
			_allocator,
			_builder_allocator_constructor
		>,
		_vertex
	>
{
	typedef typename _algorithm::template CDataStorage<
		_manager_builder_allocator_constructor<
			_manager,
			_builder,
			_allocator,
			_builder_allocator_constructor
		>,
		_vertex
	> inherited;

	typedef typename inherited::CGraphVertex CGraphVertex;
	typedef typename CGraphVertex::_index_type _index_type;

	IC CDataStorageConstructor(const u32 vertex_count) :
		inherited(vertex_count)
	{
	}
};
