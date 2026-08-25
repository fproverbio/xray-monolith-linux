////////////////////////////////////////////////////////////////////////////
//	Module 		: associative_vector_compare_predicate.h
//	Created 	: 14.10.2005
//  Modified 	: 14.10.2005
//	Author		: Dmitriy Iassenev
//	Description : associative vector compare predicate template class
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
	typename _key_type,
	typename _data_type,
	typename _compare_predicate_type
>
class associative_vector_compare_predicate : public _compare_predicate_type
{
private:
	typedef _compare_predicate_type inherited;

public:
	// Was `typedef _key_type _key_type;` etc. - a self-referential typedef
	// re-using the exact same identifier as the template parameter it
	// names. Legal C++ (name-shadowing a template parameter with an
	// identically-named member typedef is permitted) and never used
	// qualified anywhere in this codebase (grep-verified - nothing
	// references `associative_vector_compare_predicate<...>::_key_type`),
	// but GCC's -Wtemplate-body diagnostic flags the shadowing and then
	// treats the template as permanently poisoned at the first real
	// instantiation ("instantiating erroneous template"), regardless of
	// -fpermissive. Since these three typedefs were genuinely dead (no
	// real caller), the cleanest fix is removing them rather than
	// reaching for a diagnostic-suppression pragma.

public:
	typedef std::pair<_key_type, _data_type> value_type;

public:
	IC associative_vector_compare_predicate();
	IC associative_vector_compare_predicate(const _compare_predicate_type& compare_predicate);
	IC bool operator()(const _key_type& lhs, const _key_type& rhs) const;
	IC bool operator()(const value_type& lhs, const value_type& rhs) const;
	IC bool operator()(const value_type& lhs, const _key_type& rhs) const;
	IC bool operator()(const _key_type& lhs, const value_type& rhs) const;
};

#include "associative_vector_compare_predicate_inline.h"
