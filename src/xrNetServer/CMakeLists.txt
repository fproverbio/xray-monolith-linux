# DirectPlay8 dropped - see NET_Client.cpp/NET_Server.cpp header
# comments and notes §18e. NET_Client.h/NET_Server.h/NET_Shared.h/
# stdafx.h edited in place to drop DirectPlay8 members/GUIDs/includes;
# guids.cpp doesn't exist in this tree (xrSound-only pattern, xrNetServer
# never had one). The other 6 files (NET_AuthCheck, NET_Common,
# NET_Compressor, NET_Log, ip_filter, stdafx) are real/unmodified -
# structurally embedded as direct members of IPureClient/IPureServer,
# not DirectPlay8-specific.
add_library(xrNetServer STATIC)

target_include_directories(xrNetServer PUBLIC .)

target_sources_grouped(
  TARGET xrNetServer
  NAME "NetServer"
  FILES
  ip_filter.cpp
  NET_AuthCheck.cpp
  NET_Client.cpp
  NET_Common.cpp
  NET_Compressor.cpp
  NET_Log.cpp
  NET_Server.cpp
  stdafx.cpp
)

target_compile_definitions(xrNetServer PRIVATE
  XR_NETSERVER_EXPORTS
)

target_link_libraries(xrNetServer PUBLIC xrCore)
