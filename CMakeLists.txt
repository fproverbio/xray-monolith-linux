cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(XRay.Configurations)
include(utils)

project(
  Anomaly
  DESCRIPTION "X-Ray Monolith Edition for S.T.A.L.K.E.R. Anomaly (Linux/Vulkan port)"
  LANGUAGES CXX C
)

message(STATUS "X-Ray Monolith - Linux/Vulkan port")
message(STATUS "  CMake:    ${CMAKE_VERSION}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build:    ${CMAKE_BUILD_TYPE}")

include(XRay.Compiler)

# Matches src/Common.props' solution-wide <IncludePath> (applied to every
# .vcxproj via MSBuild props inheritance) - src/3rd party and sdk/include
# hold vendored third-party headers referenced by bare `<lib/header.h>`
# includes throughout the tree, not just from their own module.
include_directories(
  "${CMAKE_SOURCE_DIR}/src/3rd party"
  "${CMAKE_SOURCE_DIR}/sdk/include"
)

# Vendored 3rd-party libs that need their own build (not header-only,
# not folded into another module's sources like LZO is inside xrCore).
add_subdirectory(sdk/include/ogg/src)
add_subdirectory(sdk/include/vorbis/lib)
add_subdirectory("src/3rd party/OpenAL-new")

add_subdirectory(src)
