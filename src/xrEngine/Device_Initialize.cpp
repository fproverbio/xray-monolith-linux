#include "stdafx.h"
#include "dedicated_server_only.h"

// Real Vulkan instance/surface bootstrap - genuinely new code, not a
// mechanical Win32->SDL2 port. See playground/xray-monolith-vulkan-port-
// notes.md ("Vulkan infrastructure already vendored and verified working")
// for the volk/Vulkan-Headers usage contract this follows:
//   1. volkInitialize() must run once before any other Vulkan/volk call.
//   2. vkCreateInstance, then volkLoadInstance(instance) so every
//      subsequent instance-level call goes through function pointers
//      resolved for this specific instance.
//   3. Physical device selection, logical device creation and swapchain
//      creation are explicitly NOT this pass's job (see the notes file) -
//      the future xrRenderVK backend does that, once xrEngine itself
//      compiles. This file only gets far enough to have a VkInstance and
//      a VkSurfaceKHR for that future code to pick up (stored on Device,
//      see device.h).
//
// <volk.h> must come before <SDL_vulkan.h> below: SDL_vulkan.h only
// declares its own opaque VkInstance/VkSurfaceKHR stand-ins if a real
// vulkan.h/VULKAN_H_ hasn't already been included - volk.h pulls in the
// real vulkan/vulkan.h, so SDL_Vulkan_CreateSurface() below ends up with
// the real handle types, not SDL's placeholders.
#include <volk.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#ifdef INGAME_EDITOR
# include "../include/editor/ide.hpp"
# include "engine_impl.hpp"
#endif // #ifdef INGAME_EDITOR

#ifdef INGAME_EDITOR
void CRenderDevice::initialize_editor()
{
    m_editor_module = LoadLibrary("editor.dll");
    if (!m_editor_module)
    {
        Msg("! cannot load library \"editor.dll\"");
        return;
    }

    m_editor_initialize = (initialize_function_ptr)GetProcAddress(m_editor_module, "initialize");
    VERIFY(m_editor_initialize);

    m_editor_finalize = (finalize_function_ptr)GetProcAddress(m_editor_module, "finalize");
    VERIFY(m_editor_finalize);

    m_engine = xr_new<engine_impl>();
    m_editor_initialize(m_editor, m_engine);
    VERIFY(m_editor);

    m_hWnd = m_editor->view_handle();
    VERIFY(m_hWnd != INVALID_HANDLE_VALUE);
}
#endif // #ifdef INGAME_EDITOR

namespace
{
	// Vulkan has no single canonical VkResult->string function in the
	// headers (unlike e.g. HRESULT's FormatMessage) - this covers the
	// handful of codes vkCreateInstance/SDL_Vulkan_CreateSurface can
	// actually return, falling back to the raw numeric value for
	// anything else. Matches this codebase's existing "fatal init
	// failure" convention (R_ASSERT3(condition, message, extra-info) -
	// see xrSound/xrNetServer's own init-failure call sites) rather than
	// inventing a new error-reporting shape for this one file.
	LPCSTR VkResultToString(VkResult result, string64& buffer)
	{
		switch (result)
		{
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		default:
			xr_sprintf(buffer, sizeof(buffer), "VkResult(%d)", static_cast<int>(result));
			return buffer;
		}
	}

	// Creates m_vkInstance + m_vkSurface on Device, right after the real
	// window exists. Deliberately defensive/verbose (every VkResult
	// checked, every intermediate step logged) - this is the first real
	// Vulkan code in this port, worth being loud about exactly where it
	// gets to versus where it doesn't on a machine with no Vulkan-capable
	// windowing backend compiled into SDL2 yet (see the notes file).
	void CreateVulkanInstanceAndSurface(SDL_Window* window, LPCSTR appName)
	{
		string64 errbuf;

		R_ASSERT3(volkInitialize() == VK_SUCCESS,
			"Unable to initialize Vulkan loader (volkInitialize failed)",
			"is a Vulkan runtime (libvulkan.so.1 / an ICD) installed?");

		unsigned int extensionCount = 0;
		if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr))
		{
			// Expected, documented failure mode on this development
			// machine: SDL2 here only has the offscreen/dummy/evdev video
			// drivers compiled in (no X11 dev headers were available when
			// SDL2 was vendored - see notes file), none of which have
			// Vulkan surface support, so this call fails cleanly before
			// any instance is ever created. A real X11-capable SDL2 build
			// reaches vkCreateInstance below instead of stopping here.
			Msg("! Vulkan: SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
			Msg("! Vulkan: no Vulkan-capable SDL2 video driver available - "
				"instance/surface bootstrap stops here (expected on this "
				"machine, see playground/xray-monolith-vulkan-port-notes.md)");
			return;
		}

		xr_vector<const char*> extensions(extensionCount);
		R_ASSERT3(SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data()),
			"SDL_Vulkan_GetInstanceExtensions (name query) failed", SDL_GetError());

		Msg("* Vulkan: %u required instance extension(s):", extensionCount);
		for (unsigned int i = 0; i < extensionCount; ++i)
			Msg("*   %s", extensions[i]);

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = appName;
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 5, 3); // X-Ray Monolith v1.5.3
		appInfo.pEngineName = "X-Ray Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = extensionCount;
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkInstance instance = VK_NULL_HANDLE;
		const VkResult createResult = vkCreateInstance(&createInfo, nullptr, &instance);
		R_ASSERT3(createResult == VK_SUCCESS, "vkCreateInstance failed", VkResultToString(createResult, errbuf));

		volkLoadInstance(instance);
		Msg("* Vulkan: instance created (targeting API 1.3)");

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
		{
			Msg("! Vulkan: SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
			Msg("! Vulkan: instance created successfully, but no surface - "
				"same no-Vulkan-capable-driver limitation as above, just "
				"caught one call later");
			vkDestroyInstance(instance, nullptr);
			return;
		}

		Msg("* Vulkan: surface created");

		Device.m_vkInstance = instance;
		Device.m_vkSurface = surface;
	}
}

PROTECT_API void CRenderDevice::Initialize()
{
	Log("Initializing Engine...");
	TimerGlobal.Start();
	TimerMM.Start();

#ifdef INGAME_EDITOR
    if (strstr(Core.Params, "-editor"))
        initialize_editor();
#endif // #ifdef INGAME_EDITOR

	// Unless a substitute window has already been specified (the editor's
	// escape hatch above - always inert, see notes file on INGAME_EDITOR
	// being permanently disabled), create the real window here.
	if (m_sdlWnd == nullptr)
	{
		LPCSTR title = "S.T.A.L.K.E.R.: Anomaly";

		// SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE
		// matches OpenXRay's own CRenderDevice::Initialize() (their direct
		// SDL2 reference implementation - see notes file) - the window
		// starts hidden/borderless and CRenderDevice::Run()/Create() show
		// it and set its real style once startup finishes. The one real
		// difference from OpenXRay: SDL_WINDOW_VULKAN instead of their
		// SDL_WINDOW_OPENGL, since this port targets Vulkan, not GL.
		Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;

		m_sdlWnd = SDL_CreateWindow(title, 0, 0, 640, 480, flags);
		R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());

		// Real Vulkan instance + window surface - see
		// CreateVulkanInstanceAndSurface() above. Deliberately not fatal
		// if this can't get a Vulkan-capable surface (see that function's
		// comments) - a concrete renderer isn't expected to exist yet at
		// this stage of the port either way (see notes file), so this
		// pass only needs to prove the bootstrap code itself is correct,
		// not that it succeeds on every machine.
		CreateVulkanInstanceAndSurface(m_sdlWnd, title);
	}

	// Save window geometry - replaces GetWindowLong(GWL_STYLE) +
	// GetWindowRect + GetClientRect. m_rcWindowBounds is the window's
	// position+size on the desktop; m_rcWindowClient is its drawable area
	// (SDL2 windows have no separate title-bar/border inset to account
	// for here the way Win32's GetClientRect did - SDL_GetWindowSize()
	// already reports the drawable area).
	{
		int wx = 0, wy = 0, ww = 0, wh = 0;
		SDL_GetWindowPosition(m_sdlWnd, &wx, &wy);
		SDL_GetWindowSize(m_sdlWnd, &ww, &wh);

		m_rcWindowBounds.left = wx;
		m_rcWindowBounds.top = wy;
		m_rcWindowBounds.right = wx + ww;
		m_rcWindowBounds.bottom = wy + wh;

		m_rcWindowClient.left = 0;
		m_rcWindowClient.top = 0;
		m_rcWindowClient.right = ww;
		m_rcWindowClient.bottom = wh;
	}

	/*
	if (strstr(lpCmdLine,"-gpu_sw")!=NULL) HW.Caps.bForceGPU_SW = TRUE;
	else HW.Caps.bForceGPU_SW = FALSE;
	if (strstr(lpCmdLine,"-gpu_nopure")!=NULL) HW.Caps.bForceGPU_NonPure = TRUE;
	else HW.Caps.bForceGPU_NonPure = FALSE;
	if (strstr(lpCmdLine,"-gpu_ref")!=NULL) HW.Caps.bForceGPU_REF = TRUE;
	else HW.Caps.bForceGPU_REF = FALSE;
	*/

	Device.seqAppStart.Add(&m_imgui);
	Device.seqAppEnd.Add(&m_imgui);
}
