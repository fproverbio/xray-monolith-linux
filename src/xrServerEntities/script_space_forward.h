////////////////////////////////////////////////////////////////////////////
//	Module 		: script_space_forward.h
//	Created 	: 21.07.2004
//  Modified 	: 21.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script space forward declarations
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace luabind
{
	class object;
	template <class T>
	class functor;
	// object_cast<T>(...) used to have a forward-only declaration here
	// too, `template <class T> T object_cast(const object& obj);` - real,
	// working object_cast<T> is a 2-template-parameter function
	// (`template<class T, class ValueWrapper> T object_cast(ValueWrapper
	// const&)`, luabind/lua_proxy_interface.hpp), and this 1-parameter
	// forward declaration was NOT a redeclaration of it - it's a distinct,
	// more-specialized overload (its parameter type is fixed/non-dependent
	// rather than a second deducible template parameter). C++ partial
	// ordering always prefers the more-specialized overload when both are
	// viable, so every real object_cast<T>(some_luabind_object) call
	// anywhere in this codebase was silently resolving to THIS forward
	// declaration instead of the real, defined template - compiling fine
	// (valid overload resolution) but failing to link (no definition
	// exists anywhere for this narrower signature), even though the real
	// template's body was fully visible at every call site the whole
	// time. Confirmed via nm on the resulting .o files and an isolated
	// single-line repro; removing this declaration was sufficient by
	// itself to fix every luabind::object_cast<T>/luabind::functor<T>
	// undefined-symbol case in the project (16 symbols).
};
