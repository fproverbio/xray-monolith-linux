////////////////////////////////////////////////////////////////////////////
//	Module 		: functor.hpp
//	Description : luabind::functor<T> - a copyable handle to a Lua callable
//	              value (function, or nil), invocable like a plain C++
//	              functor returning T.
//
//	This engine's own gameplay code (xrEngine, xrServerEntities, xrGame)
//	declares `luabind::functor<T>` members and parameters throughout, and
//	several of those files already `#include <luabind/functor.hpp>`
//	expecting this exact path to resolve (see e.g.
//	src/xrServerEntities/pch_script.h, src/xrGame/GameTask.h,
//	src/xrGame/Level_input.cpp). luabind-deboostified upstream dropped the
//	type outright (doc/changes.txt: "removed functor (use object with
//	call_function() instead)"), so this header restores it as a thin,
//	self-contained wrapper built on the primitives upstream kept: adl::object,
//	the object-based call_function() overload, default_converter and
//	native_converter_base.
//
//	Adapted from OpenXRay's src/xrScriptEngine/Functor.hpp, which solves the
//	identical gap for their fork of the same upstream luabind-deboostified.
//	One deliberate difference: OpenXRay's functor<TResult, Policies...> takes
//	a variadic call-policy pack; this file keeps a single template parameter,
//	`template <class T> class functor`, to match the forward declaration
//	already vendored (unmodified, pre-dating this port) at
//	src/xrServerEntities/script_space_forward.h and relied on by every real
//	`luabind::functor<...>` call site surveyed across xrEngine/xrGame/
//	xrServerEntities - all of which instantiate it with exactly one template
//	argument. The other simplification this enables: operator() calls
//	call_function<TResult>() with the default policy list rather than
//	threading a policy_list<Policies...> through, since there is no pack to
//	forward. OpenXRay's own void-return specialization of operator() is
//	skipped too - their code already notes it "has no purpose" (a void
//	function may `return` a void-typed expression since C++03), and this
//	fork's own test suite already exercises call_function<void>(...) as a
//	matter of course (see src/3rd party/luabind/test/test_free_functions.cpp).
////////////////////////////////////////////////////////////////////////////

#ifndef LUABIND_FUNCTOR_HPP_INCLUDED
#define LUABIND_FUNCTOR_HPP_INCLUDED

#include <utility>

#include <luabind/object.hpp>
#include <luabind/from_stack.hpp>
#include <luabind/detail/format_signature.hpp>
#include <luabind/detail/conversion_policies/native_converter.hpp>

namespace luabind {

	template <class T>
	class functor : public adl::object
	{
	public:
		functor() {}
		functor(adl::object const& obj) : adl::object(obj) {}

		template <class... Args>
		T operator()(Args&&... args) const
		{
			auto const* obj = static_cast<adl::object const*>(this);
			return call_function<T>(*obj, std::forward<Args>(args)...);
		}
	};

	namespace detail {

		template <class T>
		struct type_to_string<functor<T> >
		{
			static void get(lua_State* L)
			{
				lua_pushstring(L, "function<");
				type_to_string<T>::get(L);
				lua_pushstring(L, ">");
				lua_concat(L, 3);
			}
		};

	} // namespace detail

	template <class T>
	struct default_converter<functor<T> >
		: native_converter_base<functor<T> >
	{
		static int compute_score(lua_State* L, int index)
		{
			return lua_isfunction(L, index) || lua_isnil(L, index) ? 0 : no_match;
		}

		static functor<T> to_cpp_deferred(lua_State* L, int index)
		{
			if (lua_isnil(L, index))
				return functor<T>();
			return object(from_stack(L, index));
		}

		static void to_lua_deferred(lua_State* L, functor<T> const& value)
		{
			value.push(L);
		}
	};

	template <class T>
	struct default_converter<const functor<T> >
		: default_converter<functor<T> >
	{};

	template <class T>
	struct default_converter<const functor<T>&>
		: default_converter<functor<T> >
	{};

} // namespace luabind

#endif // LUABIND_FUNCTOR_HPP_INCLUDED
