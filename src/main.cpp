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
	HWND   win32_window,
	UINT   win32_message,
	WPARAM win32_w_param,
	LPARAM win32_l_param)
{
	LRESULT res = 0;

	switch (win32_message)
	{
		case WM_DESTROY:
		case WM_CLOSE: {
			win32_running = false;
		} break;
		default: {
			res = DefWindowProcW(win32_window, win32_message, win32_w_param, win32_l_param);
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

class VulkanAllocator {
public:
	struct MemoryProperties {
		vk::MemoryPropertyFlags required;
		vk::MemoryPropertyFlags preferred;
	};

	vk::Buffer CreateBuffer(vk::Device const device, vk::BufferCreateInfo const &create_info, VulkanAllocator::MemoryProperties const memory_properties_include, VulkanAllocator::MemoryProperties const memory_properties_exclude);
	vk::Image CreateImage(vk::Device const device, vk::ImageCreateInfo const &create_info, VulkanAllocator::MemoryProperties const memory_properties_include, VulkanAllocator::MemoryProperties const memory_properties_exclude);
	void Allocate(vk::Device const device, vk::PhysicalDeviceMemoryProperties2 const &physical_device_memory_properties);

private:
	struct Allocation {
		vk::DeviceMemory memory;
		vk::DeviceSize offset;
	};
	std::unordered_map<vk::Buffer, Allocation> buffer_allocations;
	std::unordered_map<vk::Image, Allocation> image_allocations;

	struct UnallocatedBuffer {
		vk::Buffer const buffer;
		MemoryProperties const memory_properties_include;
		MemoryProperties const memory_properties_exclude;
	};
	std::vector<UnallocatedBuffer> unallocated_buffers;

	struct UnallocatedImage {
		vk::Image const image;
		MemoryProperties const memory_properties_include;
		MemoryProperties const memory_properties_exclude;
	};
	std::vector<UnallocatedImage> unallocated_images;
};

vk::Buffer VulkanAllocator::CreateBuffer(
	vk::Device const device, 
	vk::BufferCreateInfo const &create_info, 
	VulkanAllocator::MemoryProperties const memory_properties_include, 
	VulkanAllocator::MemoryProperties const memory_properties_exclude
	) {
	vk::Buffer buffer = device.createBuffer(create_info);
	this->unallocated_buffers.push_back({buffer, memory_properties_include, memory_properties_exclude});
	return buffer;
}

vk::Image VulkanAllocator::CreateImage(
	vk::Device const device, 
	vk::ImageCreateInfo const &create_info, 
	VulkanAllocator::MemoryProperties const memory_properties_include, 
	VulkanAllocator::MemoryProperties const memory_properties_exclude
	) {
	vk::Image image = device.createImage(create_info);
	this->unallocated_images.push_back({image, memory_properties_include, memory_properties_exclude});
	return image;
}

void VulkanAllocator::Allocate(vk::Device const device, vk::PhysicalDeviceMemoryProperties2 const &physical_device_memory_properties) {
	std::vector<vk::BindBufferMemoryInfo> bind_buffer_memory_infos;
	bind_buffer_memory_infos.reserve(this->unallocated_buffers.size());
	std::vector<vk::BindImageMemoryInfo> bind_image_memory_infos;
	bind_image_memory_infos.reserve(this->unallocated_images.size());
	std::vector<vk::BufferMemoryRequirementsInfo2> buffer_memory_requirements;
	buffer_memory_requirements.reserve(this->unallocated_buffers.size());
	std::vector<vk::ImageMemoryRequirementsInfo2> image_memory_requirements;
	image_memory_requirements.reserve(this->unallocated_images.size());

	for (
		size_t unallocated_buffer_idx = 0;
		unallocated_buffer_idx < this->unallocated_buffers.size();
		++unallocated_buffer_idx
		) {
		UnallocatedBuffer unallocated_buffer = this->unallocated_buffers[unallocated_buffer_idx];

		vk::MemoryDedicatedRequirements memory_dedicated_requirements;
		
		vk::BufferMemoryRequirementsInfo2 memory_requirements_info;
		memory_requirements_info.buffer = unallocated_buffer.buffer;
		memory_requirements_info.pNext = &memory_dedicated_requirements;
		
		vk::BufferMemoryRequirements2 memory_requirements = device.getBufferMemoryRequirements2(memory_requirements_info);

		if (memory_dedicated_requirements.prefersDedicatedAllocation || memory_dedicated_requirements.requiresDedicatedAllocation) {
			vk::MemoryDedicatedAllocateInfo memory_dedicated_allocate_info;
			memory_dedicated_allocate_info.buffer = unallocated_buffer.buffer;
			
			vk::MemoryAllocateInfo memory_allocate_info;
			memory_allocate_info.pNext = &memory_dedicated_allocate_info;
			memory_allocate_info.allocationSize = memory_requirements.memoryRequirements.size;
			memory_allocate_info.memoryTypeIndex = std::numeric_limits<uint32_t>::max();
			
			for (uint32_t memory_type_idx = 0; memory_type_idx < physical_device_memory_properties.memoryTypeCount; ++memory_type_idx) {
				uint32_t memory_type_bit = 1 << memory_type_idx;
				uint32_t memory_type_bits = memory_requirements.memoryRequirements.memoryTypeBits;
				
				uint32_t memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
				
				if (((memory_type_bits & memory_type_bit) != 0) &&
					((unallocated_buffer.memory_properties_include.required & memory_properties) != 0) &&
					((unallocated_buffer.memory_properties_exclude.required & memory_properties) == 0) &&
					((unallocated_buffer.memory_properties_include.preferred & memory_properties) != 0) &&
					((unallocated_buffer.memory_properties_exclude.preferred & memory_properties) == 0)
					) {
					memory_allocate_info.memoryTypeIndex = memory_type_idx;
					break;
				}
			}
			if (memory_allocate_info.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
				for (uint32_t memory_type_idx = 0; memory_type_idx < physical_device_memory_properties.memoryTypeCount; ++memory_type_idx) {
					uint32_t memory_type_bit = 1 << memory_type_idx;
					uint32_t memory_type_bits = memory_requirements.memoryRequirements.memoryTypeBits;
					
					uint32_t memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
					
					if (((memory_type_bits & memory_type_bit) != 0) &&
						((unallocated_buffer.memory_properties_include.required & memory_properties) != 0) &&
						((unallocated_buffer.memory_properties_exclude.required & memory_properties) == 0)
						) {
						memory_allocate_info.memoryTypeIndex = memory_type_idx;
						break;
					}
				}
			}
			// TODO: Throw exception if memoryTypeIndex is still not set.

			vk::DeviceMemory memory = device.AllocateMemory(memory_allocate_info);

			vk::BindBufferMemoryInfo bind_memory_info;
			bind_memory_info.buffer = unallocated_buffer.buffer;
			bind_memory_info.memory = memory;
			bind_buffer_memory_infos.push_back(bind_memory_info);

			// Replace current element with last element.
			// Easy way to remove current element without having to move around lots of elements.
			this->unallocated_buffers[unallocated_buffer_idx] = this->unallocated_buffers.back();
			--unallocated_buffer_idx;
			this->unallocated_buffers.pop_back();
		}

		buffer_memory_requirements.push_back(memory_requirements);
	}

	for (
		size_t unallocated_image_idx = 0;
		unallocated_image_idx < this->unallocated_images.size();
		++unallocated_image_idx
		) {
		UnallocatedImage unallocated_image = this->unallocated_images[unallocated_image_idx];

		vk::MemoryDedicatedRequirements memory_dedicated_requirements;
		
		vk::ImageMemoryRequirementsInfo2 memory_requirements_info;
		memory_requirements_info.image = unallocated_image.image;
		memory_requirements_info.pNext = &memory_dedicated_requirements;
		
		vk::ImageMemoryRequirements2 memory_requirements = device.getImageMemoryRequirements2(memory_requirements_info);

		if (memory_dedicated_requirements.prefersDedicatedAllocation || memory_dedicated_requirements.requiresDedicatedAllocation) {
			vk::MemoryDedicatedAllocateInfo memory_dedicated_allocate_info;
			memory_dedicated_allocate_info.image = unallocated_image.image;
			
			vk::MemoryAllocateInfo memory_allocate_info;
			memory_allocate_info.pNext = &memory_dedicated_allocate_info;
			memory_allocate_info.allocationSize = memory_requirements.memoryRequirements.size;
			memory_allocate_info.memoryTypeIndex = std::numeric_limits<uint32_t>::max();
			
			for (uint32_t memory_type_idx = 0; memory_type_idx < physical_device_memory_properties.memoryTypeCount; ++memory_type_idx) {
				uint32_t memory_type_bit = 1 << memory_type_idx;
				uint32_t memory_type_bits = memory_requirements.memoryRequirements.memoryTypeBits;
				
				uint32_t memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
				
				if (((memory_type_bits & memory_type_bit) != 0) &&
					((unallocated_image.memory_properties_include.required & memory_properties) != 0) &&
					((unallocated_image.memory_properties_exclude.required & memory_properties) == 0) &&
					((unallocated_image.memory_properties_include.preferred & memory_properties) != 0) &&
					((unallocated_image.memory_properties_exclude.preferred & memory_properties) == 0)
					) {
					memory_allocate_info.memoryTypeIndex = memory_type_idx;
					break;
				}
			}
			if (memory_allocate_info.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
				for (uint32_t memory_type_idx = 0; memory_type_idx < physical_device_memory_properties.memoryTypeCount; ++memory_type_idx) {
					uint32_t memory_type_bit = 1 << memory_type_idx;
					uint32_t memory_type_bits = memory_requirements.memoryRequirements.memoryTypeBits;
					
					uint32_t memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
					
					if (((memory_type_bits & memory_type_bit) != 0) &&
						((unallocated_image.memory_properties_include.required & memory_properties) != 0) &&
						((unallocated_image.memory_properties_exclude.required & memory_properties) == 0)
						) {
						memory_allocate_info.memoryTypeIndex = memory_type_idx;
						break;
					}
				}
			}
			// TODO: Throw exception if memoryTypeIndex is still not set.

			vk::DeviceMemory memory = device.AllocateMemory(memory_allocate_info);

			vk::BindImageMemoryInfo bind_memory_info;
			bind_memory_info.image = unallocated_image.image;
			bind_memory_info.memory = memory;
			bind_image_memory_infos.push_back(bind_memory_info);

			// Replace current element with last element.
			// Easy way to remove current element without having to move around lots of elements.
			this->unallocated_images[unallocated_image_idx] = this->unallocated_images.back();
			--unallocated_image_idx;
			this->unallocated_images.pop_back();
		}

		image_memory_requirements.push_back(memory_requirements);
	}


}

int WINAPI WinMain(
	HINSTANCE win32_instance,
	HINSTANCE win32_prev_instance,
	LPSTR     win32_command_line,
	int       win32_show_command) 
{
	UNUSED(win32_prev_instance);
	UNUSED(win32_command_line);
	UNUSED(win32_show_command);

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
	});

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
			vulkan_device_queue_infos.push_back(vk::DeviceQueueCreateInfo{
				{},
				static_cast<uint32_t>(i),
				vulkan_queue_family_properties[i].queueCount,
				vulkan_queue_priorities.data(),
			});
		}
	}

	// TODO: Device extensions.
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

	size_t vulkan_graphics_queue_family_idx = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if ((vulkan_queue_family_properties[i].queueFlags & vk::QueueFlags{vk::QueueFlagBits::eGraphics}) != vk::QueueFlags{})
		{
			vulkan_graphics_queue_family_idx = i;
			break;
		}
	}
	vk::Queue vulkan_graphics_queue = vulkan_queues[vulkan_graphics_queue_family_idx][0];

	size_t vulkan_transfer_queue_family_idx = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if ((vulkan_queue_family_properties[i].queueFlags & vk::QueueFlags{vk::QueueFlagBits::eTransfer}) != vk::QueueFlags{})
		{
			vulkan_transfer_queue_family_idx = i;
			break;
		}
	}
	vk::Queue vulkan_transfer_queue = vulkan_queues[vulkan_transfer_queue_family_idx][0];

	vk::CommandPool vulkan_graphics_command_pool = vulkan_device.createCommandPool({
		vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
		static_cast<uint32_t>(vulkan_graphics_queue_family_idx),
	});

	vk::CommandPool vulkan_transfer_command_pool;
	if (vulkan_graphics_queue_family_idx != vulkan_transfer_queue_family_idx)
	{
		vulkan_transfer_command_pool = vulkan_device.createCommandPool({
			vk::CommandPoolCreateFlags(),
			static_cast<uint32_t>(vulkan_transfer_queue_family_idx),
		});
	}
	else
	{
		vulkan_transfer_command_pool = vulkan_graphics_command_pool;
	}

	std::vector<vk::CommandBuffer> vulkan_graphics_command_buffers = vulkan_device.allocateCommandBuffers({
		vulkan_graphics_command_pool, vk::CommandBufferLevel::ePrimary, BASED_RENDERER_VULKAN_FRAME_COUNT
	});

	vk::CommandBuffer vulkan_transfer_command_buffer;
	if (vulkan_graphics_command_pool != vulkan_transfer_command_pool)
	{
		std::vector<vk::CommandBuffer> v = vulkan_device.allocateCommandBuffers({
			vulkan_transfer_command_pool, vk::CommandBufferLevel::ePrimary, 1
		});
		vulkan_transfer_command_buffer = v[0];
	}
	else
	{
		// Just don't use the transfer command buffer then!
	}

	HMONITOR win32_monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitor_info {sizeof(MONITORINFO)};
	if (!GetMonitorInfoW(win32_monitor, &monitor_info)) 
	{
	    return -3;
	}
	int32_t monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
	int32_t monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

	WNDCLASSEXW win32_window_class 
	{
		.cbSize = sizeof(WNDCLASSEXW),
		.style = 0,
		.lpfnWndProc = win32_event_callback,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = win32_instance,
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
		win32_instance,
		nullptr
	);
	if (!win32_window)
	{
		return -2;
	}

	RECT client_rect;
	GetClientRect(win32_window, &client_rect);
	int32_t client_width = client_rect.right - client_rect.left;
	int32_t client_height = client_rect.bottom - client_rect.top;

	vk::SurfaceKHR vulkan_surface = vulkan_instance.createWin32SurfaceKHR({
		{},
		win32_instance,
		win32_window,
	});

	size_t vulkan_present_queue_family_idx = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if (vulkan_physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), vulkan_surface))
		{
			vulkan_present_queue_family_idx = i;
			break;
		}
	}
	vk::Queue vulkan_present_queue = vulkan_queues[vulkan_present_queue_family_idx][0];

	std::vector<vk::SurfaceFormatKHR> vulkan_surface_formats = vulkan_physical_device.getSurfaceFormatsKHR(vulkan_surface);
  vk::Format vulkan_format = vulkan_surface_formats.front().format; // TODO

  vk::SurfaceCapabilitiesKHR vulkan_surface_capabilities = vulkan_physical_device.getSurfaceCapabilitiesKHR(vulkan_surface);
  
  vk::Extent2D vulkan_swapchain_extent;
  if (vulkan_surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() || 
  	vulkan_surface_capabilities.currentExtent.height == std::numeric_limits<uint32_t>::max())
  {
    vulkan_swapchain_extent.width  = std::clamp(static_cast<uint32_t>(client_width), vulkan_surface_capabilities.minImageExtent.width, vulkan_surface_capabilities.maxImageExtent.width);
    vulkan_swapchain_extent.height = std::clamp(static_cast<uint32_t>(client_height), vulkan_surface_capabilities.minImageExtent.height, vulkan_surface_capabilities.maxImageExtent.height);
  }
  else
  {
    vulkan_swapchain_extent = vulkan_surface_capabilities.currentExtent;
  }

  vk::PresentModeKHR vulkan_swapchain_present_mode = vk::PresentModeKHR::eFifo; // TODO

  vk::SurfaceTransformFlagBitsKHR vulkan_pre_transform = (vulkan_surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
                                                 ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                 : vulkan_surface_capabilities.currentTransform;

  vk::CompositeAlphaFlagBitsKHR vulkan_composite_alpha = (vulkan_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied : (vulkan_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied : (vulkan_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

  vk::SwapchainCreateInfoKHR vulkan_swapchain_create_info {
  	vk::SwapchainCreateFlagsKHR(),
		vulkan_surface,
    std::clamp(BASED_RENDERER_VULKAN_FRAME_COUNT, vulkan_surface_capabilities.minImageCount, vulkan_surface_capabilities.maxImageCount),
    vulkan_format,
    vk::ColorSpaceKHR::eSrgbNonlinear,
    vulkan_swapchain_extent,
    1,
    vk::ImageUsageFlagBits::eColorAttachment,
    vk::SharingMode::eExclusive,
    {},
    vulkan_pre_transform,
    vulkan_composite_alpha,
    vulkan_swapchain_present_mode,
    true,
    nullptr,
  };

  std::array<uint32_t, 2> vulkan_queue_family_indices {
  	static_cast<uint32_t>(vulkan_graphics_queue_family_idx),
  	static_cast<uint32_t>(vulkan_transfer_queue_family_idx)
  };
  if (vulkan_graphics_queue_family_idx != vulkan_transfer_queue_family_idx)
  {
	vulkan_swapchain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
	vulkan_swapchain_create_info.queueFamilyIndexCount = static_cast<uint32_t>(vulkan_queue_family_indices.size());
	vulkan_swapchain_create_info.pQueueFamilyIndices = vulkan_queue_family_indices.data();
  }

  vk::SwapchainKHR vulkan_swapchain = vulkan_device.createSwapchainKHR(vulkan_swapchain_create_info);

  std::vector<vk::Image> vulkan_swapchain_images = vulkan_device.getSwapchainImagesKHR(vulkan_swapchain);

  std::vector<vk::ImageView> vulkan_swapchain_image_views;
  vulkan_swapchain_image_views.reserve(vulkan_swapchain_images.size());
  vk::ImageViewCreateInfo vulkan_image_view_create_info {
  	{}, 
  	{},
  	vk::ImageViewType::e2D, 
  	vulkan_format, 
  	{}, 
  	{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
  };
  for (auto image : vulkan_swapchain_images)
  {
    vulkan_image_view_create_info.image = image;
    vulkan_swapchain_image_views.push_back(vulkan_device.createImageView(vulkan_image_view_create_info));
  }

	return 0;
}