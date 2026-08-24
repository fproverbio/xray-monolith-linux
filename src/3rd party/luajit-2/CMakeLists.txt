# LuaJIT 2.1 (OpenXRay/LuaJIT, branch v2.1) - replaces Monolith's stock
# vendored 2.0.4 per the explicit decision in the port notes (§11):
# upgrade to OpenXRay's fork, but force LUAJIT_DISABLE_GC64=ON. Verified
# separately that Monolith's original 2.0.4 tree has zero GC64 code (GC64
# didn't exist before 2.1) - every existing savegame today is GC32, so
# leaving GC64 enabled by default (as OpenXRay's own wrapper does) would
# silently break savegame compatibility on the version bump alone. Forcing
# it off keeps the object/pointer representation savegame-compatible while
# still getting the 2.0->2.1 language/library improvements.
#
# Adapted from OpenXRay's Externals/LuaJIT-proj/CMakeLists.txt, trimmed to
# Linux/x86-64 only (this tree, unlike OpenXRay's, only ever targets
# Linux - see XRay.Compiler.cmake's own note on the same trim). The
# two-stage bootstrap (build a host-native minilua, use it to run DynASM
# and generate the real target VM assembly, then build a host-native
# buildvm from that to generate the bytecode/FFI/library definition
# headers) is LuaJIT's actual upstream Makefile process, translated to
# CMake custom_command steps - not simplified further, this is the
# minimum real sequence LuaJIT's build needs.

cmake_minimum_required(VERSION 3.22)

project(xrLuaJIT C CXX ASM)

set(LUAJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

option(LUAJIT_DISABLE_FFI "Disable the FFI extension" OFF)
option(LUAJIT_ENABLE_LUA52COMPAT "Enable non-breaking Lua 5.2 features" OFF)
option(LUAJIT_DISABLE_JIT "Turn LuaJIT into a pure interpreter" OFF)
option(LUAJIT_DISABLE_GC64 "Disable LJ_GC64 mode for x64 - forced ON, see comment above" ON)
option(LUAJIT_USE_SYSMALLOC "Use system realloc instead of the bundled allocator" OFF)
option(LUAJIT_USE_VALGRIND "Build for running under Valgrind" OFF)
option(LUAJIT_USE_GDBJIT "Enable the GDB JIT API client" OFF)
option(LUA_USE_APICHECK "Assert on Lua/C API misuse" OFF)
option(LUA_USE_ASSERT "Assert on internal VM invariants (slow, debugging only)" OFF)
option(LUA_USE_POSIX "Use POSIX functionality" ON)
option(LUA_USE_DLOPEN "Use dlopen() to load native modules" ON)

set(CCOPT "${CMAKE_C_FLAGS} -O3 -fomit-frame-pointer -fno-stack-protector")
set(CCOPT "${CCOPT} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -U_FORTIFY_SOURCE")
string(REPLACE " " ";" CCOPT "${CCOPT}")

set(XCFLAGS "")
if(LUAJIT_DISABLE_FFI)
  list(APPEND XCFLAGS "-DLUAJIT_DISABLE_FFI")
endif()
if(LUAJIT_ENABLE_LUA52COMPAT)
  list(APPEND XCFLAGS "-DLUAJIT_ENABLE_LUA52COMPAT")
endif()
if(LUAJIT_DISABLE_JIT)
  list(APPEND XCFLAGS "-DLUAJIT_DISABLE_JIT")
endif()
if(LUAJIT_DISABLE_GC64)
  list(APPEND XCFLAGS "-DLUAJIT_DISABLE_GC64")
endif()
if(LUAJIT_USE_SYSMALLOC)
  list(APPEND XCFLAGS "-DLUAJIT_USE_SYSMALLOC")
endif()
if(LUAJIT_USE_VALGRIND)
  list(APPEND XCFLAGS "-DLUAJIT_USE_VALGRIND")
endif()
if(LUAJIT_USE_GDBJIT)
  list(APPEND XCFLAGS "-DLUAJIT_USE_GDBJIT")
endif()
if(LUA_USE_APICHECK)
  list(APPEND XCFLAGS "-DLUA_USE_APICHECK")
endif()
if(LUA_USE_ASSERT)
  list(APPEND XCFLAGS "-DLUA_USE_ASSERT")
endif()

set(CCOPTIONS "${CCOPT};${XCFLAGS}")

add_library(xrLuaJIT)
target_compile_options(xrLuaJIT PRIVATE ${CCOPTIONS})

# lj_arch.h self-reports the target via predefined macros - ask the real
# compiler rather than hardcoding, matches upstream's own approach and
# stays correct if this ever targets ARM64 (see notes §5 - already
# confirmed no showstopper there).
execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${CCOPTIONS} -E lj_arch.h -dM
  WORKING_DIRECTORY "${LUAJIT_DIR}"
  OUTPUT_VARIABLE TESTARCH_OUTPUT
  ERROR_VARIABLE TESTARCH_ERROR
)
if(NOT "${TESTARCH_ERROR}" STREQUAL "")
  message(FATAL_ERROR "lj_arch.h probe failed: ${TESTARCH_ERROR}")
endif()

if("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_X64")
  set(TARGET_LJARCH "x64")
elseif("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_ARM64")
  set(TARGET_LJARCH "arm64")
else()
  message(FATAL_ERROR "Unsupported LuaJIT target architecture:\n${TESTARCH_OUTPUT}")
endif()
list(APPEND TARGET_ARCH "-DLUAJIT_TARGET=LUAJIT_ARCH_${TARGET_LJARCH}")

set(LJVM_MODE elfasm)

set(DASM_FLAGS "")
if("${TESTARCH_OUTPUT}" MATCHES "LJ_LE 1")
  list(APPEND DASM_FLAGS "-D ENDIAN_LE")
else()
  list(APPEND DASM_FLAGS "-D ENDIAN_BE")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_BITS 64")
  list(APPEND DASM_FLAGS "-D P64")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_HASJIT 1")
  list(APPEND DASM_FLAGS "-D JIT")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_HASFFI 1")
  list(APPEND DASM_FLAGS "-D FFI")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_DUALNUM 1")
  list(APPEND DASM_FLAGS "-D DUALNUM")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_HASFPU 1")
  list(APPEND DASM_FLAGS "-D FPU")
  list(APPEND TARGET_ARCH "-DLJ_ARCH_HASFPU=1")
else()
  list(APPEND TARGET_ARCH "-DLJ_ARCH_HASFPU=0")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_ABI_SOFTFP 1")
  list(APPEND TARGET_ARCH "-DLJ_ABI_SOFTFP=1")
else()
  list(APPEND DASM_FLAGS "-D HFABI")
  list(APPEND TARGET_ARCH "-DLJ_ABI_SOFTFP=0")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_NO_UNWIND 1")
  list(APPEND DASM_FLAGS "-D NO_UNWIND")
  list(APPEND TARGET_ARCH "-DLUAJIT_NO_UNWIND")
endif()
if("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_VERSION")
  string(REGEX MATCH "LJ_ARCH_VERSION ([0-9]+)$" _ "${TESTARCH_OUTPUT}")
  list(APPEND DASM_FLAGS "-D VER=${CMAKE_MATCH_1}")
else()
  list(APPEND DASM_FLAGS "-D VER=")
endif()

set(DASM_ARCH ${TARGET_LJARCH})
if(TARGET_LJARCH STREQUAL "x64" AND NOT "${TESTARCH_OUTPUT}" MATCHES "LJ_FR2 1")
  set(DASM_ARCH "x86")
endif()

set(TARGET_XCFLAGS "-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -U_FORTIFY_SOURCE")
# HOST_ACFLAGS/HOST_ALDFLAGS get passed as a single -D<var>=<value> string
# to the minilua/buildvm sub-builds (see below) - they must stay a plain
# space-joined string here, not a CMake ;-list, or the embedded semicolons
# get interpreted as shell command separators when ninja invokes this
# through /bin/sh. (CCOPTIONS/TARGET_ARCH are lists at this point; join
# them explicitly rather than relying on ${VAR} interpolation, which just
# splices in the raw ;-separators.)
list(JOIN CCOPTIONS " " CCOPTIONS_STR)
list(JOIN TARGET_ARCH " " TARGET_ARCH_STR)
set(HOST_ACFLAGS "${CMAKE_C_FLAGS} ${CCOPTIONS_STR} ${TARGET_ARCH_STR}")
set(HOST_ALDFLAGS "${CMAKE_C_FLAGS}")
separate_arguments(DASM_FLAGS)
separate_arguments(TARGET_XCFLAGS)

target_compile_options(xrLuaJIT PRIVATE ${TARGET_XCFLAGS})

set(DASM_DASC "${LUAJIT_DIR}/vm_${DASM_ARCH}.dasc")
set(DASM "${LUAJIT_DIR}/../dynasm/dynasm.lua")
set(BUILDVM_ARCH "${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h")

# Stage 1: a host-native minilua, just enough of an interpreter to run
# DynASM (dynasm.lua) against itself - LuaJIT can't use a *real* Lua/
# LuaJIT to bootstrap itself, hence the standalone minilua.c.
set(MINILUA_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/minilua")
set(MINILUA_FILE "${MINILUA_BINARY_DIR}/minilua")
add_custom_command(
  OUTPUT "${MINILUA_FILE}"
  COMMAND ${CMAKE_COMMAND}
    -B${MINILUA_BINARY_DIR}
    -G${CMAKE_GENERATOR}
    -S${CMAKE_CURRENT_SOURCE_DIR}/HostBuildTools/minilua
    -DCMAKE_BUILD_TYPE=Release
    -DLUAJIT_DIR=${LUAJIT_DIR}
    -DLUA_USE_POSIX=${LUA_USE_POSIX}
    "-DHOST_ACFLAGS=${HOST_ACFLAGS}"
    "-DHOST_ALDFLAGS=${HOST_ALDFLAGS}"
  COMMAND ${CMAKE_COMMAND} --build ${MINILUA_BINARY_DIR} --config Release
)
add_custom_command(OUTPUT ${BUILDVM_ARCH}
  COMMAND "${MINILUA_FILE}" ${DASM} ${DASM_FLAGS} -o ${BUILDVM_ARCH} ${DASM_DASC}
  DEPENDS "${MINILUA_FILE}"
)
add_custom_target(buildvm_arch DEPENDS ${BUILDVM_ARCH})

# Stage 2: a host-native buildvm, which turns the DynASM-generated target
# assembly (lj_vm.S) and the VM's own C sources into the actual VM
# entry-point object file plus a handful of generated headers
# (bytecode/FFI/library call-dispatch tables).
set(BUILDVM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/buildvm")
set(BUILDVM_FILE "${BUILDVM_BINARY_DIR}/buildvm")
add_custom_command(
  OUTPUT "${BUILDVM_FILE}"
  COMMAND ${CMAKE_COMMAND}
    -B${BUILDVM_BINARY_DIR}
    -G${CMAKE_GENERATOR}
    -S${CMAKE_CURRENT_SOURCE_DIR}/HostBuildTools/buildvm
    -DCMAKE_BUILD_TYPE=Release
    -DLUAJIT_DIR=${LUAJIT_DIR}
    -DBUILDVM_ARCH=${BUILDVM_ARCH}
    "-DHOST_ACFLAGS=${HOST_ACFLAGS}"
    "-DHOST_ALDFLAGS=${HOST_ALDFLAGS}"
  COMMAND ${CMAKE_COMMAND} --build ${BUILDVM_BINARY_DIR} --config Release
)
add_custom_target(buildvm DEPENDS "${BUILDVM_FILE}")
add_dependencies(buildvm buildvm_arch)

list(APPEND LJLIB_C
  "${LUAJIT_DIR}/lib_base.c"
  "${LUAJIT_DIR}/lib_bit.c"
  "${LUAJIT_DIR}/lib_debug.c"
  "${LUAJIT_DIR}/lib_ffi.c"
  "${LUAJIT_DIR}/lib_io.c"
  "${LUAJIT_DIR}/lib_jit.c"
  "${LUAJIT_DIR}/lib_math.c"
  "${LUAJIT_DIR}/lib_os.c"
  "${LUAJIT_DIR}/lib_package.c"
  "${LUAJIT_DIR}/lib_string.c"
  "${LUAJIT_DIR}/lib_table.c"
)

macro(add_buildvm_target target mode)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target}"
    COMMAND "${BUILDVM_FILE}" -m ${mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${target} ${ARGN}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS buildvm ${ARGN}
  )
endmacro()

add_buildvm_target(lj_vm.S ${LJVM_MODE})
# NOT "LANGUAGE CXX" here (OpenXRay's own template sets that, but it's
# wrong for this file - lj_vm.S is real GNU-assembler-with-cpp syntax
# (.type foo, @function etc), and forcing the C++ frontend on it parses
# that as broken C++ instead. project(... ASM) above already gives CMake
# native .S handling (cpp preprocess + as), which is what this needs.
add_buildvm_target("lj_bcdef.h" "bcdef" ${LJLIB_C})
add_buildvm_target("lj_ffdef.h" "ffdef" ${LJLIB_C})
add_buildvm_target("lj_libdef.h" "libdef" ${LJLIB_C})
add_buildvm_target("lj_recdef.h" "recdef" ${LJLIB_C})
add_buildvm_target("lj_folddef.h" "folddef" "${LUAJIT_DIR}/lj_opt_fold.c")
add_buildvm_target("jit/vmdef.lua" "libvm" ${LJLIB_C})

target_sources(xrLuaJIT PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.S"
  "${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h"
  "${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h"
  "${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h"
  "${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h"
  "${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h"
  ${LJLIB_C}
  "${LUAJIT_DIR}/lib_aux.c"
  "${LUAJIT_DIR}/lib_init.c"
  "${LUAJIT_DIR}/lj_alloc.c"
  "${LUAJIT_DIR}/lj_api.c"
  "${LUAJIT_DIR}/lj_asm.c"
  "${LUAJIT_DIR}/lj_assert.c"
  "${LUAJIT_DIR}/lj_bc.c"
  "${LUAJIT_DIR}/lj_bcread.c"
  "${LUAJIT_DIR}/lj_bcwrite.c"
  "${LUAJIT_DIR}/lj_buf.c"
  "${LUAJIT_DIR}/lj_carith.c"
  "${LUAJIT_DIR}/lj_ccall.c"
  "${LUAJIT_DIR}/lj_ccallback.c"
  "${LUAJIT_DIR}/lj_cconv.c"
  "${LUAJIT_DIR}/lj_cdata.c"
  "${LUAJIT_DIR}/lj_char.c"
  "${LUAJIT_DIR}/lj_clib.c"
  "${LUAJIT_DIR}/lj_cparse.c"
  "${LUAJIT_DIR}/lj_crecord.c"
  "${LUAJIT_DIR}/lj_ctype.c"
  "${LUAJIT_DIR}/lj_debug.c"
  "${LUAJIT_DIR}/lj_dispatch.c"
  "${LUAJIT_DIR}/lj_err.c"
  "${LUAJIT_DIR}/lj_ffrecord.c"
  "${LUAJIT_DIR}/lj_func.c"
  "${LUAJIT_DIR}/lj_gc.c"
  "${LUAJIT_DIR}/lj_gdbjit.c"
  "${LUAJIT_DIR}/lj_ir.c"
  "${LUAJIT_DIR}/lj_lex.c"
  "${LUAJIT_DIR}/lj_lib.c"
  "${LUAJIT_DIR}/lj_load.c"
  "${LUAJIT_DIR}/lj_mcode.c"
  "${LUAJIT_DIR}/lj_meta.c"
  "${LUAJIT_DIR}/lj_obj.c"
  "${LUAJIT_DIR}/lj_opt_dce.c"
  "${LUAJIT_DIR}/lj_opt_fold.c"
  "${LUAJIT_DIR}/lj_opt_loop.c"
  "${LUAJIT_DIR}/lj_opt_mem.c"
  "${LUAJIT_DIR}/lj_opt_narrow.c"
  "${LUAJIT_DIR}/lj_opt_sink.c"
  "${LUAJIT_DIR}/lj_opt_split.c"
  "${LUAJIT_DIR}/lj_parse.c"
  "${LUAJIT_DIR}/lj_prng.c"
  "${LUAJIT_DIR}/lj_profile.c"
  "${LUAJIT_DIR}/lj_record.c"
  "${LUAJIT_DIR}/lj_snap.c"
  "${LUAJIT_DIR}/lj_state.c"
  "${LUAJIT_DIR}/lj_str.c"
  "${LUAJIT_DIR}/lj_strfmt.c"
  "${LUAJIT_DIR}/lj_strfmt_num.c"
  "${LUAJIT_DIR}/lj_strscan.c"
  "${LUAJIT_DIR}/lj_tab.c"
  "${LUAJIT_DIR}/lj_trace.c"
  "${LUAJIT_DIR}/lj_udata.c"
  "${LUAJIT_DIR}/lj_vmevent.c"
  "${LUAJIT_DIR}/lj_vmmath.c"
)

target_include_directories(xrLuaJIT
  PUBLIC "${LUAJIT_DIR}"
  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(xrLuaJIT PRIVATE
  $<$<BOOL:${LUA_USE_POSIX}>:m>
  $<$<BOOL:${LUA_USE_DLOPEN}>:dl>
)

target_compile_options(xrLuaJIT PRIVATE -Wno-comment)

set_target_properties(xrLuaJIT PROPERTIES PREFIX "")
