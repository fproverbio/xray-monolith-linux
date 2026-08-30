////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_help.cpp
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Script Engine help
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#ifdef DEBUG

#include <luabind/detail/object.hpp>
#include <luabind/lua_proxy_interface.hpp>
#include <luabind/detail/call.hpp>
#include <luabind/function.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/class_registry.hpp>
#include <luabind/detail/debug.hpp>

xr_string to_string					(::luabind::object const& o)
{
	using namespace luabind;
	if (luabind::type(o) == LUA_TSTRING) return object_cast<luabind::string>(o).c_str();
	lua_State* L = o.interpreter();
	LUABIND_CHECK_STACK(L);

	if (luabind::type(o) == LUA_TNUMBER)
	{
		char buffer[64];
		xr_sprintf(buffer, sizeof(buffer), "%.16g", object_cast<float>(o));
		return buffer;
	}

	return xr_string("<") + lua_typename(L, luabind::type(o)) + ">";
}

void strreplaceall						(xr_string &str, LPCSTR S, LPCSTR N)
{
	LPCSTR	A;
	int		S_len = xr_strlen(S);
	while ((A = strstr(str.c_str(),S)) != 0)
		str.replace(A - str.c_str(),S_len,N);
}

xr_string &process_signature				(xr_string &str)
{
	strreplaceall	(str,"custom [","");
	strreplaceall	(str,"]","");
	strreplaceall	(str,"float","number");
	strreplaceall	(str,"lua_State*, ","");
	strreplaceall	(str," ,lua_State*","");
	return			(str);
}

// True if the Lua value on top of the stack is a closure produced by luabind's
// make_function_aux() (upvalue 2 is the function_tag/function_tag_ndef marker).
static bool is_luabind_object_function	(::luabind::object const& fn)
{
	lua_State* L = fn.interpreter();
	fn.push(L);
	bool result = luabind::detail::is_luabind_function(L, -1);
	lua_pop(L, 1);
	return result;
}

// Extracts the function_object* stored in upvalue 1 of a luabind-generated closure.
static ::luabind::detail::function_object *function_chain	(::luabind::object const& fn)
{
	using namespace luabind;
	return *touserdata<detail::function_object*>(std::get<1>(getupvalue(fn, 1)));
}

// Walks the overload chain (function_object::next), collecting one processed
// signature string per overload.
static void collect_signatures				(lua_State *L, ::luabind::detail::function_object const* chain, xr_vector<xr_string> &out)
{
	for (::luabind::detail::function_object const* i = chain; i != 0; i = i->next)
	{
		i->format_signature(L, "");
		luabind::detail::stack_pop pop(L, 1);
		LPCSTR sig = lua_tostring(L, -1);
		xr_string s(sig ? sig : "");
		process_signature(s);
		out.push_back(s);
	}
}

xr_string member_to_string			(::luabind::object const& e, LPCSTR function_signature)
{
	using namespace luabind;
	lua_State* L = e.interpreter();
	LUABIND_CHECK_STACK(L);

	if (luabind::type(e) == LUA_TFUNCTION)
	{
		if (!is_luabind_object_function(e))
			return to_string(e);

		xr_vector<xr_string> sigs;
		collect_signatures(L, function_chain(e), sigs);

		xr_string s;
		for (xr_vector<xr_string>::const_iterator i = sigs.begin(); i != sigs.end(); ++i)
		{
			if (i != sigs.begin())
				s += "\n";
			s += function_signature;
			s += " ";
			s += *i;
			s += ";";
		}
		return s;
	}

    return to_string(e);
}

void print_class						(lua_State *L, ::luabind::detail::class_rep *crep)
{
	using namespace luabind;
	xr_string			S;
	// print class and bases
	{
		S				= (crep->get_class_type() != detail::class_rep::cpp_class) ? "LUA class " : "C++ class ";
		S.append		(crep->name());
		typedef luabind::vector<detail::class_rep::base_info> BASES;
		const BASES &bases = crep->bases();
		BASES::const_iterator	I = bases.begin(), B = I;
		BASES::const_iterator	E = bases.end();
		if (B != E)
			S.append	(" : ");
		for ( ; I != E; ++I) {
			if (I != B)
				S.append(",");
			S.append	((*I).base->name());
		}
		Msg				("%s {",S.c_str());
	}
	// print class constants
	{
		const luabind::map<const char*, int, detail::ltstr>	&constants = crep->static_constants();
		luabind::map<const char*, int, detail::ltstr>::const_iterator	I = constants.begin();
		luabind::map<const char*, int, detail::ltstr>::const_iterator	E = constants.end();
		for ( ; I != E; ++I)
			Msg		("    const %s = %d;",(*I).first,(*I).second);
		if (!constants.empty())
			Msg		("    ");
	}
	// print class properties
	// (property_tag is the marker function used as the lua_CFunction entry
	// point for every property closure created via luabind::property())
	{
		crep->get_table	(L);
		detail::stack_pop	pop_table(L, 1);
		object				table(from_stack(L, -1));

		bool any = false;
		iterator e;
		for (iterator i(table); i != e; ++i) {
			if (luabind::type(*i) != LUA_TFUNCTION)
				continue;
			object obj = *i;
			obj.push(L);
			bool is_property = (lua_tocfunction(L, -1) == &detail::property_tag);
			lua_pop(L, 1);
			if (!is_property)
				continue;
			Msg	("    property %s;", to_string(i.key()).c_str());
			any = true;
		}
		if (any)
			Msg		("    ");
	}
	// print class constructors
	// (constructors are registered as a regular luabind function overload
	// chain stored under the "__init" key of the class' method table)
	{
		crep->get_table	(L);
		detail::stack_pop	pop_table(L, 1);
		lua_pushliteral	(L, "__init");
		lua_gettable	(L, -2);
		detail::stack_pop	pop_init(L, 1);

		if (detail::is_luabind_function(L, -1)) {
			object				init_fn(from_stack(L, -1));
			xr_vector<xr_string>	sigs;
			collect_signatures	(L, function_chain(init_fn), sigs);
			for (xr_vector<xr_string>::const_iterator i = sigs.begin(); i != sigs.end(); ++i)
				Msg		("    %s%s;",crep->name(),i->c_str());
			if (!sigs.empty())
				Msg		("    ");
		}
	}
	// print class methods
	{
		crep->get_table	(L);
		detail::stack_pop	pop_table(L, 1);
		object				table(from_stack(L, -1));

		iterator e;
		for (iterator i(table); i != e; ++i) {
			if (luabind::type(*i) != LUA_TFUNCTION)
				continue;

			object obj = *i;
			obj.push(L);
			bool is_property = (lua_tocfunction(L, -1) == &detail::property_tag);
			lua_pop(L, 1);
			if (is_property)
				continue;

			xr_string	S;
			S			= "    function ";
			S.append	(to_string(i.key()).c_str());

			strreplaceall	(S,"function __add","operator +");
			strreplaceall	(S,"function __sub","operator -");
			strreplaceall	(S,"function __mul","operator *");
			strreplaceall	(S,"function __div","operator /");
			strreplaceall	(S,"function __pow","operator ^");
			strreplaceall	(S,"function __lt","operator <");
			strreplaceall	(S,"function __le","operator <=");
			strreplaceall	(S,"function __gt","operator >");
			strreplaceall	(S,"function __ge","operator >=");
			strreplaceall	(S,"function __eq","operator ==");
			Msg			("%s",member_to_string(obj,S.c_str()).c_str());
		}
	}
	Msg			("};\n");
}

void print_free_functions				(lua_State *L, const ::luabind::object &ns_table, LPCSTR header, const xr_string &indent)
{
	using namespace luabind;
	u32							count = 0;
	{
		iterator	e;
		for (iterator i(ns_table); i != e; ++i) {
			if (luabind::type(*i) != LUA_TFUNCTION)
				continue;

			object fn = *i;
			if (!is_luabind_object_function(fn))
				continue;

			if (!count)
				Msg("\n%snamespace %s {",indent.c_str(),header);
			++count;

			xr_string				fname = to_string(i.key());
			xr_vector<xr_string>	sigs;
			collect_signatures		(L, function_chain(fn), sigs);
			for (xr_vector<xr_string>::const_iterator si = sigs.begin(); si != sigs.end(); ++si)
				Msg("    %sfunction %s%s;",indent.c_str(),fname.c_str(),si->c_str());
		}
	}
	{
		xr_string				_indent = indent;
		_indent.append			("    ");
		ns_table.push			(L);
		detail::stack_pop		pop_table(L, 1);
		lua_pushnil		(L);
		while (lua_next(L, -2) != 0) {
			if (lua_type(L, -1) == LUA_TTABLE) {
				LPCSTR			S = lua_tostring(L, -2);
				if (xr_strcmp("_G",S) && xr_strcmp("package",S)) {
					object		child(from_stack(L, -1));
					print_free_functions(L,child,S,_indent);
				}
			}
#pragma todo("Dima to Dima : Remove this hack if find out why")
			if (lua_isnumber(L,-2)) {
				lua_pop(L,1);
				lua_pop(L,1);
				break;
			}
			lua_pop	(L, 1);
		}
	}
	if (count)
		Msg("%s};",indent.c_str());
}

void print_help							(lua_State *L)
{
	using namespace luabind;
	Msg					("\nList of the classes exported to LUA\n");
	{
		detail::class_registry						*registry = detail::class_registry::get_registry(L);
		luabind::map<type_id, detail::class_rep*> const	&classes = registry->get_classes();
		luabind::map<type_id, detail::class_rep*>::const_iterator	I = classes.begin();
		luabind::map<type_id, detail::class_rep*>::const_iterator	E = classes.end();
		for ( ; I != E; ++I)
			print_class(L, I->second);
	}
	Msg					("End of list of the classes exported to LUA\n");
	Msg					("\nList of the namespaces exported to LUA\n");
	print_free_functions(L,globals(L),"","");
	Msg					("End of list of the namespaces exported to LUA\n");
}
#else
void print_help(lua_State* L)
{
	Msg("! Release build doesn't support lua-help :(");
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
