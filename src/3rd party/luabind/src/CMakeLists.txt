add_library(xrLuabind)

target_sources(xrLuabind PRIVATE
	class.cpp
	class_info.cpp
	class_registry.cpp
	class_rep.cpp
	create_class.cpp
	error.cpp
	exception_handler.cpp
	function.cpp
	function_introspection.cpp
	headertest.cpp
	inheritance.cpp
	link_compatibility.cpp
	memory.cpp
	object_rep.cpp
	open.cpp
	operator.cpp
	pcall.cpp
	scope.cpp
	set_package_preload.cpp
	stack_content_by_name.cpp
	weak_ref.cpp
	wrapper_base.cpp
)

target_include_directories(xrLuabind
	PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(xrLuabind
	PUBLIC
	xrLuabindApi
)

if (XRAY_USE_LUAJIT)
    target_link_libraries(xrLuabind PUBLIC xrLuaJIT)
else()
    find_package(Lua REQUIRED)
    target_link_libraries(xrLuabind PUBLIC ${LUA_LIBRARIES})
endif()

target_compile_definitions(xrLuabind
	PUBLIC
	$<$<BOOL:${BUILD_SHARED_LIBS}>:LUABIND_DYNAMIC_LINK>
)

# xrEngine/stdafx.h defines LUABIND_NO_EXCEPTIONS/BOOST_NO_EXCEPTIONS itself
# for `!defined(DEBUG) || defined(FORCE_NO_EXCEPTIONS)` (FORCE_NO_EXCEPTIONS
# is never defined anywhere in this tree, so that reduces to "non-Debug
# config"), and every engine TU picks that up by including stdafx.h. This
# library's own sources (error.cpp in particular) never include stdafx.h,
# so without this they silently compile the opposite (exceptions-based)
# branch of luabind/error.hpp and never emit get_error_callback() /
# set_error_callback() / set_error_callback_not_crash() /
# get_error_callback_not_crash() / get_cast_failed_callback() /
# set_cast_failed_callback() - which engine code expects and calls,
# causing undefined references at final link. PRIVATE (not PUBLIC): engine
# consumers already get the matching definition via stdafx.h, and
# propagating this as a -D flag on top of that would just conflict with
# stdafx.h's own #define of the same macros.
target_compile_definitions(xrLuabind
	PRIVATE
	$<$<NOT:$<CONFIG:Debug>>:LUABIND_NO_EXCEPTIONS>
	$<$<NOT:$<CONFIG:Debug>>:BOOST_NO_EXCEPTIONS>
)

set_target_properties(xrLuabind PROPERTIES
	PREFIX ""
)

if (BUILD_SHARED_LIBS)
	install(TARGETS xrLuabind LIBRARY
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	)
endif()
