# Vendored Dear ImGui (docking branch, v1.91.8) for the engine's in-game
# debug UI overlay (src/xrEngine/imgui_base.cpp / imgui_helper.h / .cpp;
# imgui_base_input.cpp also #includes imgui.h but stays excluded from the
# xrEngine build for an unrelated reason - see src/xrEngine/CMakeLists.txt's
# own comment: it directly #includes ../xrGame/xr_level_controller.h and
# ../xrGame/UICursor.h, which don't exist yet - a separate blocker from
# imgui itself).
#
# Same situation as giflib (sdk/include/giflib/CMakeLists.txt, commit
# e1447d9e): real upstream Dear ImGui source is *already* vendored in this
# repo at ../../../src/3rd party/imgui (wired into the Windows build via
# src/xrEngine/xrEngine.vcxproj's own AdditionalIncludeDirectories entry
# for that path, plus src/Common.props's shared IncludePath for its
# parent dir - see below - both untouched by this port, i.e. this is the
# original upstream layout, not something invented for Linux). It is a
# complete, unmodified, real copy of the Dear ImGui *docking* branch:
# imgui.h defines IMGUI_VERSION "1.91.8" / IMGUI_VERSION_NUM 19180 and
# both IMGUI_HAS_DOCK and IMGUI_HAS_VIEWPORT (docking-branch-only macros)
# right at the top of the file - not the vanilla/master branch. That
# matters here: imgui_base.cpp calls ImGui::DockSpace() /
# ImGuiDockNodeFlags_PassthruCentralNode and sets
# ImGuiConfigFlags_DockingEnable, all docking-branch-only API, so this
# vendored copy is exactly what those call sites need - nothing to
# re-vendor or upgrade/downgrade.
#
# So, like giflib, this does NOT re-vendor a second copy of the source
# under sdk/include/imgui/ - it builds the real, already-vendored source
# in place via a relative path, the same in-place-build precedent as
# src/3rd party/SDL2/CMakeLists.txt, src/3rd party/Vulkan/CMakeLists.txt,
# etc. sdk/include/imgui/ exists purely to hold this build file, matching
# the sdk/include/<lib>/ naming/location convention, without duplicating
# any source.
#
# Two PUBLIC include dirs are needed, matching the two real #include
# conventions actually used by src/xrEngine's own imgui_*.cpp files:
#   - #include <imgui.h> / <imgui_internal.h>   (bare, no dir prefix)
#     needs the imgui source dir itself on the path - this is what
#     xrEngine.vcxproj's own AdditionalIncludeDirectories add
#     ("$(SolutionDir)3rd party\imgui\").
#   - #include <imgui/IconsFontAwesome6.h>      (with an "imgui/" prefix)
#     needs imgui's *parent* dir ("3rd party") on the path - this is what
#     src/Common.props's shared <IncludePath> adds
#     ("$(SolutionDir)..\src\3rd party"), applied to every project.
# Both are exposed below so consumers don't have to know which form a
# given #include uses.
#
# File set: the four core .cpp files (imgui.cpp/imgui_draw.cpp/
# imgui_widgets.cpp/imgui_tables.cpp) plus imgui_demo.cpp - NOT actually
# optional here, despite demo windows usually being the first thing cut
# from a vendored ImGui build: imgui_base.cpp's debug menu really does
# call ImGui::ShowDemoWindow() (a dev-tools "Demo" menu item, guarded by
# a plain bool toggle), which is defined in imgui_demo.cpp, so leaving it
# out would be an immediate link error - plus imgui_custom.cpp, which is
# NOT optional either: imconfig.h sets IMGUI_INCLUDE_IMGUI_USER_H /
# IMGUI_USER_H_FILENAME "imgui_custom.h", so imgui.h itself #includes
# imgui_custom.h at the bottom of every translation unit that includes
# imgui.h anywhere in the tree, unconditionally declaring
# ImGui::LockMousePos() - it's part of this vendored copy's core, not a
# backend/platform file, so it ships with the library rather than being
# left for a consumer to provide.
#
# Explicitly excluded:
#   - backends/imgui_impl_dx9/dx10/dx11.cpp/.h: Direct3D 9/10/11 renderer
#     backends. Nothing in src/xrEngine #includes any imgui_impl_*.h
#     (confirmed by grep), and backend integration - actually feeding
#     ImGui's draw data into a renderer - needs a real working Vulkan
#     renderer to make sense, which doesn't exist yet: out of scope for
#     this pass, same call as leaving egif_lib.c out of the giflib
#     target.
#   - imgui.vcxproj/.vcxproj.filters/.natvis/.natstepfilter: MSVC project
#     scaffolding, not source.
#
# No hand-written config header needed: imconfig.h (already present next
# to imgui.h) is used as-is. It sets IMGUI_DEFINE_MATH_OPERATORS,
# IMGUI_DISABLE_DEFAULT_FONT, IMGUI_DISABLE_DEFAULT_ALLOCATORS (the
# engine calls ImGui::SetAllocatorFunctions() itself in imgui_base.cpp
# before CreateContext(), so this is already satisfied on the consumer
# side) and the imgui_custom.h user-header hook described above - none
# of that is Windows-specific. The one _WIN32-adjacent bit in imconfig.h
# (a `#pragma comment(lib, "xrCore.lib")` under `#ifdef
# USE_VERIFY_IN_RELEASE`) is inert unless USE_VERIFY_IN_RELEASE is
# defined, which nothing here defines.
set(IMGUI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/3rd party/imgui")

add_library(imgui STATIC
  "${IMGUI_SRC_DIR}/imgui.cpp"
  "${IMGUI_SRC_DIR}/imgui_draw.cpp"
  "${IMGUI_SRC_DIR}/imgui_widgets.cpp"
  "${IMGUI_SRC_DIR}/imgui_tables.cpp"
  "${IMGUI_SRC_DIR}/imgui_demo.cpp"
  "${IMGUI_SRC_DIR}/imgui_custom.cpp"
)

target_include_directories(imgui PUBLIC
  "${IMGUI_SRC_DIR}"
  "${IMGUI_SRC_DIR}/.."
)

# Not inherited from the top-level project when this file is exercised
# standalone (e.g. a throwaway verification harness), and Dear ImGui
# itself needs at least C++11 - match luabind's precedent
# (src/3rd party/luabind/CMakeLists.txt: set(CMAKE_CXX_STANDARD 17)) of
# not relying on the including project to have picked a standard already.
target_compile_features(imgui PUBLIC cxx_std_17)
