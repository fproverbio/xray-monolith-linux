# Vendored Vulkan-Headers + volk (real upstream source, both pinned to
# vulkan-sdk-1.4.321.0 - see NOTES.txt in this directory for the exact
# commits and the reasoning below).
#
# This machine has the Vulkan *runtime* loader (libvulkan.so.1, Mesa
# drivers) but no Vulkan development headers anywhere under /usr/include,
# no pkg-config binary at all, and no unversioned libvulkan.so symlink -
# only the versioned libvulkan.so.1. That combination means a normal
# find_package(Vulkan)/-lvulkan link would fail here (there's nothing for
# the linker to resolve -lvulkan against), even though the runtime is
# perfectly capable of loading and running Vulkan programs.
#
# volk (https://github.com/zeux/volk, Arseny Kapoulkine) is the standard
# fix for exactly this situation: instead of linking against libvulkan.so
# at build time, it dlopen()s the loader at process startup and resolves
# every Vulkan entry point (vkCreateInstance, vkGetInstanceProcAddr, all
# the rest) as a function pointer at runtime. No -lvulkan, no dev
# symlink, no pkg-config needed - just libdl (via CMAKE_DL_LIBS) and
# Vulkan-Headers to compile against.
#
# Vulkan-Headers (https://github.com/KhronosGroup/Vulkan-Headers) is
# header-only and Apache-2.0/MIT licensed; only its include/ tree is
# vendored here (registry XML, docs, tests, BUILD.gn etc. from upstream
# are dropped - not needed to compile against).
#
# VK_USE_PLATFORM_XLIB_KHR is set PUBLIC on the loader target: this port
# targets X11/Xlib windowing via SDL2. X11 core dev headers are present
# on this machine (/usr/include/X11) but Wayland dev headers are not, so
# Xlib is the right surface extension for now. Any consumer that links
# against VulkanLoader automatically gets VK_USE_PLATFORM_XLIB_KHR and
# therefore vkCreateXlibSurfaceKHR/VkXlibSurfaceCreateInfoKHR etc.
# declared in vulkan_core.h.
#
# Usage note for future call sites (not exercised by this CMakeLists.txt,
# just documented here since it's easy to get wrong with volk):
#   1. volkInitialize() must be called once, before any other Vulkan or
#      volk call (including vkCreateInstance) - it dlopen()s the loader
#      and resolves the global/instance-independent function pointers
#      (vkGetInstanceProcAddr, vkCreateInstance, ...).
#   2. After vkCreateInstance, call volkLoadInstance(instance) (or
#      volkLoadInstanceOnly) so subsequent instance/physical-device calls
#      go through function pointers specialized for that instance.
#   3. After vkCreateDevice, call volkLoadDevice(device) - and for best
#      performance when using a single VkDevice, prefer
#      volkLoadDeviceTable(&table, device) or re-run volkLoadDevice so
#      device-level calls (vkCmd*, vkQueueSubmit, ...) skip the internal
#      instance->device dispatch indirection volk otherwise uses.

# --- Vulkan-Headers: header-only, INTERFACE target ---
add_library(Vulkan-Headers INTERFACE)
add_library(Vulkan::Headers ALIAS Vulkan-Headers)
target_include_directories(Vulkan-Headers INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers/include"
)

# --- volk: real static-lib target, dlopen-based Vulkan loader ---
add_library(VulkanLoader STATIC
  volk/volk.c
)
add_library(volk::volk ALIAS VulkanLoader)

target_include_directories(VulkanLoader PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/volk"
)

target_link_libraries(VulkanLoader PUBLIC Vulkan-Headers ${CMAKE_DL_LIBS})

target_compile_definitions(VulkanLoader PUBLIC VK_USE_PLATFORM_XLIB_KHR)
