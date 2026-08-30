#include "../../build_config_defines.h"

#ifdef USE_LUAJIT_ONE
#pragma comment(lib, "LuaJIT-1.1.8.lib")
#else
#pragma comment(lib, "lua51.lib")
#endif //-USE_LUAJIT_ONE

#include "lua.hpp"

//#pragma comment(lib, "xrCore.lib")
//#pragma comment(lib, "user32.lib")

extern "C"{
    #include "lfs.h"
    #include "lmarshal.h"
}

//#include "Libs.h"
#include "script_additional_libs.h"

static const struct luaL_Reg R[] =
{
	{ NULL,	    NULL },
};

// script_storage.cpp calls luaopen_lua_extensions() unconditionally on
// every Lua VM init - real, mandatory startup dependency, not optional.
// Its actual body only ever needed open_additional_libs()/luaopen_lfs()/
// luaopen_marshal() (all real, portable, wired in below) plus LuaJIT's
// own already-built-in luaopen_jit()/luaopen_ffi()/luaopen_debug() under
// IsDebug - luasocket/luapanda (included here originally just for the 2
// functions below) were never actually called from this function at all.
//extern "C" __declspec(dllexport)
int luaopen_lua_extensions(lua_State *L, bool IsDebug = false){

    open_additional_libs(L);

    luaopen_lfs(L);
    //open_string(L);
    //open_math(L);
    //open_table(L);
    luaopen_marshal(L);
    //open_kb(L);
    //open_log(L);

    if (IsDebug)
    {
        luaopen_jit(L);
        luaopen_ffi(L);
        luaopen_debug(L);
    }

	luaL_register(L, "lua_extensions", R);
	return 0;
}

// luaopen_socket_core_init()/pdebug_init_init() - only ever called from
// script_storage.cpp's `lua_debug` debug-only branch (LuaPanda remote
// debugger support), never from real gameplay code. Real bodies need
// luasocket (a genuine ~38-file third-party BSD-sockets library,
// lua-extensions/luasocket/ - already has a real Unix backend upstream,
// usocket.c, just never wired into this port's build) and luapanda.cpp
// (a real LuaPanda/VSCode remote-debugger integration) - both left
// unbuilt as a deliberately-scoped-out, real, larger follow-up (not a
// permanent architectural exclusion like the renderer/MP gaps elsewhere
// in this port - just genuinely bigger than "wire in what already
// compiles", see the notes file). Stubbed here exactly like
// Layers/xrAPI/render_stub.cpp's philosophy: real feature, not reachable
// unless a user explicitly opts into `lua_debug`, safe to no-op for now.
static int luaopen_socket_core_stub(lua_State* L) {
	lua_newtable(L);
	return 1;
}

lua_CFunction luaopen_socket_core_init() {
	return luaopen_socket_core_stub;
}

void pdebug_init_init(lua_State* /*L*/) {
}
