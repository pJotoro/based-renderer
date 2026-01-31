#define UNUSED(X) (void)(X)

#define BASED_RENDERER_DEBUG 1

#define BASED_RENDERER_VULKAN_DEBUG BASED_RENDERER_DEBUG
#define BASED_RENDERER_VULKAN_VALIDATION BASED_RENDERER_VULKAN_DEBUG
#define BASED_RENDERER_VULKAN_DEBUG_OUTPUT BASED_RENDERER_VULKAN_DEBUG
#define BASED_RENDERER_VULKAN_LAYERS (BASED_RENDERER_VULKAN_DEBUG || BASED_RENDERER_VULKAN_VALIDATION)

#define BASED_RENDERER_VULKAN_FRAME_COUNT 2

#define BASED_RENDERER_FULLSCREEN 0

#define VK_KHR_platform_surface "VK_KHR_win32_surface"

#include "pch.hpp"

// Works just like std::print, except it prints to the debug console.
template<class... Args> void 
dprint(std::format_string<Args...> fmt, Args&&... args) 
{
	std::string s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringA(s.c_str());
}

// Same, but the format string is a wide string.
template<class... Args> void 
dprint(std::wformat_string<Args...> fmt, Args&&... args) 
{
	std::wstring s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringW(s.c_str());
}

// TODO: Remove global.
static bool win32_running;

LRESULT WINAPI win32_event_callback(
	HWND   hWnd,
	UINT   Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	LRESULT res = 0;

	switch (Msg)
	{
		case WM_DESTROY:
		case WM_CLOSE: {
			win32_running = false;
		} break;
		default: {
			res = DefWindowProcW(hWnd, Msg, wParam, lParam);
		} break;
	}

	return res;
}

vk::Bool32 VKAPI_PTR vulkan_debug_callback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
	vk::DebugUtilsMessageTypeFlagsEXT message_types,
	const vk::DebugUtilsMessengerCallbackDataEXT * callback_data,
	void *user_data)
{
	UNUSED(message_severity);
	UNUSED(message_types);
	UNUSED(user_data);

	dprint("{}\n", callback_data->pMessage);

	return vk::False;
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd) 
{
	UNUSED(hPrevInstance);
	UNUSED(lpCmdLine);
	UNUSED(nShowCmd);

	vk::ApplicationInfo vulkan_app_info {
		"based_renderer",
		VK_API_VERSION_1_0,
		"based_renderer",
		VK_API_VERSION_1_0,
		VK_API_VERSION_1_4,
	};

#if BASED_RENDERER_VULKAN_LAYERS
	std::vector<const char*> vulkan_layers;
#if BASED_RENDERER_VULKAN_DEBUG
	vulkan_layers.push_back("VK_LAYER_LUNARG_monitor");
#endif
#if BASED_RENDERER_VULKAN_VALIDATION
	vulkan_layers.push_back("VK_LAYER_KHRONOS_validation");
	std::array<vk::ValidationFeatureEnableEXT, 2> vulkan_enabled_validation_features 
	{
		vk::ValidationFeatureEnableEXT::eBestPractices,
		vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
	};
	vk::ValidationFeaturesEXT vulkan_validation_features 
	{
		vulkan_enabled_validation_features,
	};
#endif
#endif // BASED_RENDERER_VULKAN_LAYERS

#if BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vk::DebugUtilsMessengerCreateInfoEXT vulkan_debug_output_info {
		{},
		{
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
		},
		{
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		},
		vulkan_debug_callback
	};
#endif

	std::vector<const char *> vulkan_extensions;
	vulkan_extensions.push_back("VK_KHR_surface");
	vulkan_extensions.push_back(VK_KHR_platform_surface);
#if BASED_RENDERER_VULKAN_LAYERS
	vulkan_extensions.push_back("VK_EXT_layer_settings");
#endif
#if BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vulkan_extensions.push_back("VK_EXT_debug_utils");
#endif

	vk::InstanceCreateInfo vulkan_instance_create_info 
	{
		{},
		&vulkan_app_info,
#if BASED_RENDERER_VULKAN_LAYERS
		static_cast<uint32_t>(vulkan_layers.size()),
		vulkan_layers.data(),
#else
		0,
		nullptr,
#endif
		static_cast<uint32_t>(vulkan_extensions.size()),
		vulkan_extensions.data(),
	};

#if BASED_RENDERER_VULKAN_VALIDATION && BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vulkan_instance_create_info.pNext = &vulkan_validation_features;
	vulkan_validation_features.pNext = &vulkan_debug_output_info;
#elif BASED_RENDERER_VULKAN_VALIDATION
	vulkan_instance_create_info.pNext = &vulkan_validation_features;
#elif BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vulkan_instance_create_info.pNext = &vulkan_debug_output_info;
#endif

	vk::Instance vulkan_instance = vk::createInstance(vulkan_instance_create_info);

	// Choose the first discrete GPU.
	// If there is no discrete GPU, default to the last GPU.
	std::vector<vk::PhysicalDevice> vulkan_physical_devices = vulkan_instance.enumeratePhysicalDevices();
	vk::PhysicalDevice vulkan_physical_device = *std::find_if(vulkan_physical_devices.begin(), vulkan_physical_devices.end(), 
		[](vk::PhysicalDevice p) {
			vk::PhysicalDeviceProperties props = p.getProperties();
			return props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		}
	);

	std::vector<vk::QueueFamilyProperties> vulkan_queue_family_properties = vulkan_physical_device.getQueueFamilyProperties();

	// TODO: This is stupid. Find out how queue priorities should be done.
	std::array<float, 64> vulkan_queue_priorities {
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	std::vector<vk::DeviceQueueCreateInfo> vulkan_device_queue_infos;
	vulkan_device_queue_infos.reserve(vulkan_queue_family_properties.size());
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if (vulkan_queue_family_properties[i].queueCount > 0)
		{
			vulkan_device_queue_infos.push_back(
				vk::DeviceQueueCreateInfo 
				{
					{},
					static_cast<uint32_t>(i),
					vulkan_queue_family_properties[i].queueCount,
					vulkan_queue_priorities.data(),
				}
			);
		}
	}

	vk::Device vulkan_device = vulkan_physical_device.createDevice(vk::DeviceCreateInfo({}, vulkan_device_queue_infos));

	// Each queue family gets its own std::vector, whether or not it has any queues.
	std::vector<std::vector<vk::Queue>> vulkan_queues{vulkan_queue_family_properties.size()};
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		vulkan_queues[i].resize(vulkan_queue_family_properties[i].queueCount);
		for (size_t j = 0; j < static_cast<size_t>(vulkan_queue_family_properties[i].queueCount); ++j)
		{
			vulkan_queues[i][j] = vulkan_device.getQueue(static_cast<uint32_t>(i), static_cast<uint32_t>(j));
		}
	}

	// TODO: Remove the requirement that the graphics queue must also be capable of transfer.
	vk::Queue vulkan_graphics_queue;
	size_t vulkan_graphics_queue_family_idx = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if ((vulkan_queue_family_properties[i].queueFlags & vk::QueueFlags{vk::QueueFlagBits::eGraphics|vk::QueueFlagBits::eTransfer}) != vk::QueueFlags{})
		{
			vulkan_graphics_queue = vulkan_queues[i][0];
			vulkan_graphics_queue_family_idx = i;
			break;
		}
	}

	vk::CommandPool vulkan_command_pool = vulkan_device.createCommandPool(
 		vk::CommandPoolCreateInfo {
 			vk::CommandPoolCreateFlags(), 
 			static_cast<uint32_t>(vulkan_graphics_queue_family_idx),
 		}
 	);

	std::vector<vk::CommandBuffer> vulkan_command_buffers = vulkan_device.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo(vulkan_command_pool, vk::CommandBufferLevel::ePrimary, BASED_RENDERER_VULKAN_FRAME_COUNT)
	);

	HMONITOR win32_monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitor_info {sizeof(MONITORINFO)};
	if (!GetMonitorInfoW(win32_monitor, &monitor_info)) {
	    return -3;
	}
	auto monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
	auto monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

	WNDCLASSEXW win32_window_class 
	{
		.cbSize = sizeof(WNDCLASSEXW),
		.style = 0,
		.lpfnWndProc = win32_event_callback,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = nullptr,
		.hCursor = nullptr,
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		.lpszClassName = L"based_renderer",
		.hIconSm = nullptr,
	};
	if (!RegisterClassExW(&win32_window_class))
	{
		return -1;
	}

	HWND win32_window = CreateWindowExW(
#if !BASED_RENDERER_FULLSCREEN
		0,
#else
		WS_EX_TOPMOST,
#endif
		L"based_renderer",
		L"based_renderer",
#if !BASED_RENDERER_FULLSCREEN
		WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
		monitor_width/4,
		monitor_height/4,
		monitor_width/2,
		monitor_height/2,
#else
		WS_POPUP,
		0,
		0,
		monitor_width,
		monitor_height,
#endif
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	if (!win32_window)
	{
		return -2;
	}


	
	return 0;
}