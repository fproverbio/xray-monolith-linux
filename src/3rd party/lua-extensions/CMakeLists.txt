# lua-extensions - a real, never-built-at-all module. script_storage.cpp
# calls luaopen_lua_extensions() unconditionally on every single Lua VM
# init (not optional/debug-only) - the whole engine can't start scripting
# at all without it, which is exactly why its absence surfaced as a
# link-time gap rather than something reachable-only-in-some-config.
#
# base.cpp - project-specific glue (not upstream-vendored), edited in
# place: luaopen_lua_extensions() itself (the mandatory part) only ever
# called open_additional_libs()/luaopen_lfs()/luaopen_marshal() plus
# LuaJIT's own already-built-in luaopen_jit()/luaopen_ffi()/luaopen_debug()
# under IsDebug - real, portable, all wired in below. Its two OTHER
# functions, luaopen_socket_core_init()/pdebug_init_init(), are only ever
# called from script_storage.cpp's separate `lua_debug` debug-only branch
# (LuaPanda remote-debugger support) - stubbed in base.cpp itself (see
# that file's own comment) rather than porting the two real, much larger
# dependencies they need:
#   - luasocket/ (~38 files, real BSD-sockets Lua library - already has
#     a working Unix backend upstream, usocket.c, just never wired in)
#   - luapanda.cpp (real LuaPanda/VSCode remote-debugger integration)
# Both stay on disk, unbuilt - a real, scoped, larger follow-up if remote
# Lua debugging is ever wanted, not a permanent architectural exclusion
# like the renderer/MP gaps elsewhere in this port.
#
# lfs.c (LuaFileSystem) and lmarshal.c (Lua serialization) are genuine
# upstream third-party C libraries, already properly cross-platform
# (lfs.c has real #ifdef _WIN32/#else POSIX branches throughout) -
# compiled as-is, no changes needed.
add_library(lua-extensions STATIC)

target_include_directories(lua-extensions PUBLIC .)

target_sources_grouped(
  TARGET lua-extensions
  NAME "LuaExtensions"
  FILES
  base.cpp
  lfs.c
  lmarshal.c
  script_additional_libs.cpp
)

target_link_libraries(lua-extensions PUBLIC xrLuaJIT)
