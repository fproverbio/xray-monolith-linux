add_library(xrCore STATIC)

target_include_directories(xrCore PUBLIC .)

# Excluded from this first pass, not deleted - real Windows-only code that
# needs its own porting decision, not a blind include:
#   blackbox/*.cpp        - Windows minidump crash-reporter UI (uses
#                            DbgHelp/TLHELP32/windows.h directly throughout).
#                            Linux replacement is a separate task (e.g. a
#                            backtrace()-based handler, same idea already
#                            planned for the stackwalker replacement - see
#                            notes section 10e).
#   FileSystem_borland.cpp - despite the name, unconditionally includes
#                            <io.h>/<Shlobj.h> with no __BORLANDC__ guard;
#                            a real Windows-only duplicate of logic that
#                            likely already lives in FileSystem.cpp.
#   xrMemory_subst_borland.cpp - entirely `#ifdef __BORLANDC__`, compiles
#                            to nothing on any compiler we support; no
#                            reason to include it at all.
target_sources_grouped(
  TARGET xrCore
  NAME "Core"
  FILES
  FS.cpp
  FTimer.cpp
  FileSystem.cpp
  LocatorAPI.cpp
  LocatorAPI_Notifications.cpp
  LocatorAPI_auth.cpp
  LocatorAPI_defs.cpp
  Lock.cpp
  LzHuf.cpp
  Model.cpp
  NET_utils.cpp
  ScopeLock.cpp
  ShaderSourceCRC.cpp
  Xr_ini.cpp
  _compressed_normal.cpp
  _math.cpp
  _sphere.cpp
  _std_extensions.cpp
  clsid.cpp
  cpuid.cpp
  crc32.cpp
  dump_string.cpp
  file_stream_reader.cpp
  log.cpp
  lzo_compressor.cpp
  memory_allocation_stats.cpp
  memory_monitor.cpp
  memory_usage.cpp
  mezz_stringbuffer.cpp
  os_clipboard.cpp
  ppmd_compressor.cpp
  rt_compressor.cpp
  rt_compressor9.cpp
  rt_lzo1x_1.cpp
  rt_lzo1x_9x.cpp
  rt_lzo1x_d1.cpp
  rt_lzo1x_d2.cpp
  rt_lzo1x_d3.cpp
  rt_lzo_init.cpp
  stdafx.cpp
  stream_reader.cpp
  string_concatenations.cpp
  xrCore.cpp
  xrDebug.cpp
  xrDebugNew.cpp
  xrMemory.cpp
  xrMemory_POOL.cpp
  xrMemory_align.cpp
  xrMemory_debug.cpp
  xrMemory_pso_Copy.cpp
  xrMemory_pso_Fill.cpp
  xrMemory_pso_Fill32.cpp
  xrMemory_subst_msvc.cpp
  xrSyncronize.cpp
  xr_shared.cpp
  xr_trims.cpp
  xrsharedmem.cpp
  xrstring.cpp
)

# XRCORE_STATIC: the original codebase already has this exact escape
# hatch for a statically-linked xrCore (guards out DllMain and other
# DLL-only code via #ifndef XRCORE_STATIC) - just needed to actually be
# defined, since we build xrCore as a static library, not a DLL.
target_compile_definitions(xrCore PRIVATE
  PURE_ALLOC
  _USRDLL
  XRCORE_EXPORTS
  XRCORE_STATIC
)
