////////////////////////////////////////////////////////////////////////////
//	Module 		: static_cast_checked.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : checked static_cast implementation for debug purposes
////////////////////////////////////////////////////////////////////////////

#ifndef STATIC_CAST_CHECKED_HPP_INCLUDED
#define STATIC_CAST_CHECKED_HPP_INCLUDED

// Needed directly for object_type_traits::remove_pointer/remove_reference,
// used below - this file has no other includes to bring it in transitively.
#include "../xrServerEntities/object_type_traits.h"

#ifdef DEBUG

namespace debug {
namespace detail {
namespace static_cast_checked {

template <typename destination_type>
struct value {
	template <typename source_type>
	inline static void check		(source_type *source)
	{
		VERIFY		(smart_cast<destination_type>(source) == static_cast<destination_type>(source));
	}

	template <typename source_type>
	inline static void check		(source_type &source)
	{
		VERIFY		(&smart_cast<destination_type>(source) == &static_cast<destination_type>(source));
	}

};

// A primary member template plus a nested `template <> ... check<false>`
// explicit specialization here is illegal: explicit specialization of a
// member template is only legal once the enclosing class template
// (helper<source_type, destination_type>) is no longer dependent, and here
// it still is. MSVC accepts this as a non-conforming extension (same as it
// does everywhere else this port has hit the pattern); GCC's
// -Wtemplate-body rejects it. Same bug class as object_comparer.h's
// CHelper/CHelper4 (notes section 27d) - rewritten with `if constexpr`
// (C++17, already the project's standard) instead of a nested
// specialization; identical compile-time branch, standard-legal.
template <typename source_type, typename destination_type>
struct helper {
	template <bool is_polymorphic>
	inline static void check		(source_type source)
	{
		if constexpr (is_polymorphic)
		{
			value<
				destination_type
			>::check	(source);
		}
	}
};

} // namespace static_cast_checked
} // namespace detail
} // namespace debug

// Both overloads below needed two established first-instantiation-template
// fixes (notes section 16/17a/17d/21c/26b/27d): `typename` before the two
// dependent typedefs (object_type_traits::remove_pointer<source_type>::type
// et al depend on this function template's own parameter), and a
// `.template` disambiguator on `helper<...>::check<...>` - `check` is a
// template member of a type still dependent on this function's own
// template parameters, so unqualified `<` after `::check` parses as
// less-than without it.
template <typename destination_type, typename source_type>
inline destination_type static_cast_checked	(source_type const & source)
{
	typedef typename object_type_traits::remove_pointer<source_type>::type			pointerless_type;
	typedef typename object_type_traits::remove_reference<pointerless_type>::type	pure_source_type;

	debug::detail::static_cast_checked::helper<
		source_type const &,
		destination_type
	>::template check<
		is_polymorphic<
			pure_source_type
		>::result
	>				(source);

	return			(static_cast<destination_type>(source));
}

template <typename destination_type, typename source_type>
inline destination_type static_cast_checked	(source_type & source)
{
	typedef typename object_type_traits::remove_pointer<source_type>::type			pointerless_type;
	typedef typename object_type_traits::remove_reference<pointerless_type>::type	pure_source_type;

	debug::detail::static_cast_checked::helper<
		source_type &,
		destination_type
	>::template check<
		is_polymorphic<
			pure_source_type
		>::result
	>				(source);

	return			(static_cast<destination_type>(source));
}

#else // #ifdef DEBUG
#	define static_cast_checked	static_cast
#endif // #ifdef DEBUG

#endif // STATIC_CAST_CHECKED_HPP_INCLUDED
