#include "pch.hpp"

// TODO: Would it make sense to add BASED_RENDERER_ to these macro names?
#define UNUSED(X) (void)(X)
#define STRINGIFY(x) #x
#define STMT(X) do {X} while (0)

// TODO: Would it make sense to not define these directly, but instead define them in CMakePresets and CMakeUserPresets?

#ifdef _DEBUG
#define BASED_RENDERER_DEBUG 1
#else
#define BASED_RENDERER_DEBUG 0
#endif

#define BASED_RENDERER_VULKAN_DEBUG BASED_RENDERER_DEBUG
#define BASED_RENDERER_VULKAN_VALIDATION BASED_RENDERER_VULKAN_DEBUG
#define BASED_RENDERER_VULKAN_DEBUG_OUTPUT BASED_RENDERER_VULKAN_DEBUG
#define BASED_RENDERER_VULKAN_LAYERS (BASED_RENDERER_VULKAN_DEBUG || BASED_RENDERER_VULKAN_VALIDATION)
#define BASED_RENDERER_VULKAN_DISABLE_PIPELINE_OPTIMIZATION BASED_RENDERER_VULKAN_DEBUG

#define BASED_RENDERER_SLANG_DEBUG BASED_RENDERER_DEBUG
#define BASED_RENDERER_SLANG_SPIRV_VALIDATION BASED_RENDERER_SLANG_DEBUG

#define BASED_RENDERER_FULLSCREEN !BASED_RENDERER_DEBUG

// TODO: What about other systems?
#define VK_KHR_platform_surface "VK_KHR_win32_surface"

// TODO: How does printing to the debug output work on other systems?

// Works just like std::print, except it prints to the debug console.
template<class... Args> 
static void dprint(std::format_string<Args...> fmt, Args&&... args) noexcept
{
	std::string s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringA(s.c_str());
}

// Same, but the format string is a wide string.
template<class... Args>
static void dprint(std::wformat_string<Args...> fmt, Args&&... args) noexcept
{
	std::wstring s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringW(s.c_str());
}

// A clever way I found to remove an element from an std::vector.
// Assumes that i is within the bounds of v.
template <class T>
static void unordered_remove(std::vector<T> &v, size_t const i) noexcept
{
	v[i] = v.back();
	v.pop_back();
}

static std::string to_string(std::vector<std::string> const &v) noexcept
{
	std::string res;
	if (v.size() > 0)
	{
		for (size_t i = 0; i < v.size() - 1; ++i)
		{
			res += v[i] + ", ";
		}
		res += v.back();
	}
	return res;
}

// static std::string read_entire_file(std::string const &path)
// {
// 	std::string res;

// 	std::ifstream file(path);
// 	if (!file)
// 	{
// 		std::string error = "Failed to load " + path + ".";
// 		throw std::runtime_error{error};
// 	}

// 	std::ostringstream buffer;
// 	buffer << file.rdbuf();
// 	res = buffer.str();

// 	return res;
// }

// TODO: What about system errors on other systems?
// TODO: Is there a cross-platform way to get the last error?
static std::system_error win32_system_error() noexcept
{
	std::error_code error_code{static_cast<int>(GetLastError()), std::system_category()};
	std::system_error system_error{error_code};
	return system_error;
}

// TODO: Remove global.
static bool win32_running;

LRESULT WINAPI win32_event_callback(
	HWND   win32_window,
	UINT   win32_message,
	WPARAM win32_w_param,
	LPARAM win32_l_param) noexcept
{
	LRESULT res = 0;

	switch (win32_message)
	{
		case WM_DESTROY:
		case WM_CLOSE: 
		{
			win32_running = false;
		} break;
		default: 
		{
			res = DefWindowProcW(win32_window, win32_message, win32_w_param, win32_l_param);
		} break;
	}

	return res;
}

static void win32_message_box(
	char const *message,
	char const *title) noexcept
{
	MessageBoxA(
		nullptr,
		message,
		title,
		MB_OK
	);
}

vk::Bool32 VKAPI_PTR vulkan_debug_callback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
	vk::DebugUtilsMessageTypeFlagsEXT message_types,
	vk::DebugUtilsMessengerCallbackDataEXT const *callback_data,
	void *user_data) noexcept
{
	UNUSED(message_severity);
	UNUSED(message_types);
	UNUSED(user_data);

	dprint("{}\n", callback_data->pMessage);

	return vk::False;
}

struct VulkanMemoryProperties 
{
	vk::MemoryPropertyFlags required;
	vk::MemoryPropertyFlags preferred;
};

static uint32_t vulkan_find_memory_type_idx(
	vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
	uint32_t const memory_type_bits,
	VulkanMemoryProperties const memory_properties_include,
	VulkanMemoryProperties const memory_properties_exclude) 
{
	// We first attempt to find a perfect match, that is, matching not only the
	// required memory properties, but also the preferred ones.
	// If that doesn't work, we then try again, but this time without the
	// preferred ones. If that doesn't work, we throw an exception.

	for (
		uint32_t memory_type_idx = 0; 
		memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
		++memory_type_idx) 
	{
		uint32_t memory_type_bit = 1 << memory_type_idx;
		
		vk::MemoryPropertyFlags memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
		
		if ((memory_type_bits & memory_type_bit) &&
			(memory_properties_include.required & memory_properties) &&
			(memory_properties_exclude.required & memory_properties) &&
			(memory_properties_include.preferred & memory_properties) &&
			(memory_properties_exclude.preferred & memory_properties)) 
		{
			return memory_type_idx;
		}
	}

	for (
		uint32_t memory_type_idx = 0; 
		memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
		++memory_type_idx) 
	{
		uint32_t memory_type_bit = 1 << memory_type_idx;
		
		vk::MemoryPropertyFlags memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
		
		if (((memory_type_bits & memory_type_bit) != 0) &&
			((memory_properties_include.required & memory_properties) != vk::MemoryPropertyFlags{}) &&
			((memory_properties_exclude.required & memory_properties) == vk::MemoryPropertyFlags{})) 
		{
			return memory_type_idx;
		}
	}

	throw vk::LogicError{"Failed to find a memory type index with the required memory properties."};
}

struct VulkanBufferAllocation 
{
	// in
	VulkanMemoryProperties memory_properties_include;
	VulkanMemoryProperties memory_properties_exclude;
	vk::Buffer buffer;

	 // out
	vk::DeviceMemory memory;
	vk::DeviceSize offset;
	vk::DeviceSize size;
	vk::DeviceSize align;
	uint32_t memory_type_idx;
	bool dedicated_allocation;
};

struct VulkanImageAllocation 
{
	// in
	VulkanMemoryProperties memory_properties_include;
	VulkanMemoryProperties memory_properties_exclude;
	vk::Image image;

	// out
	vk::DeviceMemory memory;
	vk::DeviceSize offset;
	vk::DeviceSize size;
	vk::DeviceSize align;
	uint32_t memory_type_idx;
	bool dedicated_allocation;
};

// How to use:
// 1. Create all the buffers and images you want.
// 2. Decide which memory properties you want each of them to have (or not). 
//    For example, a vertex buffer should probably be device local, while a
//	  staging buffer should be host visible.
// 3. Now you can call vulkan_allocate. When passing the buffers and images,
//	  you should only fill out the fields under the "in" comment.
// 4. For each buffer and image, the "out" fields will be filled. In all
//	  likelihood, you will only ever need to use the "memory" and "offset"
//	  fields, but the others are there as well just in case.
void vulkan_allocate(
	vk::Device const device,
	vk::PhysicalDeviceMemoryProperties2 const &physical_device_memory_properties,
	std::span<VulkanBufferAllocation> buffer_allocations,
	std::span<VulkanImageAllocation> image_allocations) 
{
	std::vector<vk::BindBufferMemoryInfo> bind_buffer_memory_infos;
	bind_buffer_memory_infos.reserve(buffer_allocations.size());
	std::vector<vk::BindImageMemoryInfo> bind_image_memory_infos;
	bind_image_memory_infos.reserve(image_allocations.size());

	for (VulkanBufferAllocation &buffer_allocation : buffer_allocations) 
	{
		vk::MemoryDedicatedRequirements memory_dedicated_requirements;
		
		vk::BufferMemoryRequirementsInfo2 memory_requirements_info;
		memory_requirements_info.buffer = buffer_allocation.buffer;
		memory_requirements_info.pNext = &memory_dedicated_requirements;
		
		vk::MemoryRequirements2 buffer_memory_requirements = device.getBufferMemoryRequirements2(memory_requirements_info);

		buffer_allocation.size = buffer_memory_requirements.memoryRequirements.size;
		buffer_allocation.align = buffer_memory_requirements.memoryRequirements.alignment;
		buffer_allocation.memory_type_idx = vulkan_find_memory_type_idx(
			physical_device_memory_properties.memoryProperties,
			buffer_memory_requirements.memoryRequirements.memoryTypeBits,
			buffer_allocation.memory_properties_include,
			buffer_allocation.memory_properties_exclude);

		if (memory_dedicated_requirements.prefersDedicatedAllocation || memory_dedicated_requirements.requiresDedicatedAllocation) 
		{
			buffer_allocation.dedicated_allocation = true;
			buffer_allocation.offset = 0;

			vk::MemoryDedicatedAllocateInfo memory_dedicated_allocate_info;
			memory_dedicated_allocate_info.buffer = buffer_allocation.buffer;
			
			vk::MemoryAllocateInfo memory_allocate_info;
			memory_allocate_info.pNext = &memory_dedicated_allocate_info;
			memory_allocate_info.allocationSize = buffer_allocation.size;
			memory_allocate_info.memoryTypeIndex = buffer_allocation.memory_type_idx;

			buffer_allocation.memory = device.allocateMemory(memory_allocate_info);

			vk::BindBufferMemoryInfo bind_buffer_memory_info;
			bind_buffer_memory_info.buffer = buffer_allocation.buffer;
			bind_buffer_memory_info.memory = buffer_allocation.memory;
			bind_buffer_memory_infos.push_back(bind_buffer_memory_info);
		}
	}

	for (VulkanImageAllocation &image_allocation : image_allocations) 
	{
		vk::MemoryDedicatedRequirements memory_dedicated_requirements;
		
		vk::ImageMemoryRequirementsInfo2 memory_requirements_info;
		memory_requirements_info.image = image_allocation.image;
		memory_requirements_info.pNext = &memory_dedicated_requirements;
		
		vk::MemoryRequirements2 image_memory_requirements = device.getImageMemoryRequirements2(memory_requirements_info);

		image_allocation.size = image_memory_requirements.memoryRequirements.size;
		image_allocation.align = image_memory_requirements.memoryRequirements.alignment;
		image_allocation.memory_type_idx = vulkan_find_memory_type_idx(
			physical_device_memory_properties.memoryProperties,
			image_memory_requirements.memoryRequirements.memoryTypeBits,
			image_allocation.memory_properties_include,
			image_allocation.memory_properties_exclude);

		if (memory_dedicated_requirements.prefersDedicatedAllocation || memory_dedicated_requirements.requiresDedicatedAllocation) 
		{
			image_allocation.dedicated_allocation = true;
			image_allocation.offset = 0;

			vk::MemoryDedicatedAllocateInfo memory_dedicated_allocate_info;
			memory_dedicated_allocate_info.image = image_allocation.image;
			
			vk::MemoryAllocateInfo memory_allocate_info;
			memory_allocate_info.pNext = &memory_dedicated_allocate_info;
			memory_allocate_info.allocationSize = image_allocation.size;
			memory_allocate_info.memoryTypeIndex = image_allocation.memory_type_idx;

			image_allocation.memory = device.allocateMemory(memory_allocate_info);

			vk::BindImageMemoryInfo bind_image_memory_info;
			bind_image_memory_info.image = image_allocation.image;
			bind_image_memory_info.memory = image_allocation.memory;
			bind_image_memory_infos.push_back(bind_image_memory_info);
		}
	}

	for (
		uint32_t memory_type_idx = 0; 
		memory_type_idx < physical_device_memory_properties.memoryProperties.memoryTypeCount; 
		++memory_type_idx)
	{
		size_t bind_buffer_memory_infos_size = bind_buffer_memory_infos.size();
		size_t bind_image_memory_infos_size = bind_image_memory_infos.size();

		vk::DeviceSize memory_offset = 0;

		// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/#heading-2-5
		auto is_power_of_2 = [](vk::DeviceSize const x) 
		{
			return (x & (x-1)) == 0;
		};
		auto align_forward = [is_power_of_2](vk::DeviceSize offset, vk::DeviceSize const align) 
		{
			if (!is_power_of_2(align))
			{
				std::string message{std::format("{} is not a power of 2.", align)};
				throw vk::LogicError{message};
			}

			// Same as (offset % align) but faster as 'align' is a power of two
			vk::DeviceSize modulo = offset & (align-1);

			if (modulo != 0) 
			{
				// If 'offset' is not aligned, push it to the
				// next value which is aligned
				offset += align - modulo;
			}
			return offset;
		};

		for (VulkanBufferAllocation const &buffer_allocation : buffer_allocations) 
		{
			if (buffer_allocation.memory_type_idx == memory_type_idx && !buffer_allocation.memory) 
			{
				memory_offset = align_forward(memory_offset, buffer_allocation.align);

				vk::BindBufferMemoryInfo bind_buffer_memory_info;
				bind_buffer_memory_info.buffer = buffer_allocation.buffer;
				bind_buffer_memory_info.memoryOffset = memory_offset;
				bind_buffer_memory_infos.push_back(bind_buffer_memory_info);

				memory_offset += buffer_allocation.size;
			}
		}

		for (VulkanImageAllocation const &image_allocation : image_allocations) 
		{
			if (image_allocation.memory_type_idx == memory_type_idx && !image_allocation.memory) 
			{
				memory_offset = align_forward(memory_offset, image_allocation.align);

				vk::BindImageMemoryInfo bind_image_memory_info;
				bind_image_memory_info.image = image_allocation.image;
				bind_image_memory_info.memoryOffset = memory_offset;
				bind_image_memory_infos.push_back(bind_image_memory_info);

				memory_offset += image_allocation.size;
			}
		}

		vk::MemoryAllocateInfo memory_allocate_info;
		memory_allocate_info.allocationSize = memory_offset;
		memory_allocate_info.memoryTypeIndex = memory_type_idx;
		vk::DeviceMemory memory = device.allocateMemory(memory_allocate_info);

		for (VulkanBufferAllocation &buffer_allocation : buffer_allocations) 
		{
			if (buffer_allocation.memory_type_idx == memory_type_idx && !buffer_allocation.memory) 
			{
				buffer_allocation.memory = memory;
			}
		}
		for (VulkanImageAllocation &image_allocation : image_allocations) 
		{
			if (image_allocation.memory_type_idx == memory_type_idx && !image_allocation.memory) 
			{
				image_allocation.memory = memory;
			}
		}
		for (size_t i = bind_buffer_memory_infos_size; i < bind_buffer_memory_infos.size(); ++i) 
		{
			bind_buffer_memory_infos[i].memory = memory;
		}
		for (size_t i = bind_image_memory_infos_size; i < bind_image_memory_infos.size(); ++i) 
		{
			bind_image_memory_infos[i].memory = memory;
		}
	}

	device.bindBufferMemory2(bind_buffer_memory_infos);
	device.bindImageMemory2(bind_image_memory_infos);
}

// TODO: Find a way to not have to write std::format("{}, {}, {}", __FUNCTION__, __LINE__, over and over again.
// TODO: Change the formatting to make it look more like a compile error.
#define SLANG_CHECK(RESULT) STMT( \
	switch (RESULT) \
	{ \
		case SLANG_OK: \
			break; \
		case SLANG_FAIL: \
			throw std::runtime_error{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: failed for unknown reason.")}; \
		case SLANG_E_NOT_IMPLEMENTED: \
			throw std::logic_error{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: function not implemented.")}; \
		case SLANG_E_NO_INTERFACE: \
			throw std::logic_error{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: no interface.")}; \
		case SLANG_E_ABORT: \
			throw std::runtime_error{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: error was aborted.")}; \
		case SLANG_E_INVALID_HANDLE: \
			throw std::logic_error{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: invalid handle")}; \
		case SLANG_E_INVALID_ARG: \
			throw std::invalid_argument{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: invalid argument.")}; \
		case SLANG_E_OUT_OF_MEMORY: \
			throw std::runtime_error{std::format("{}, {}, {}", __FUNCTION__, __LINE__, "Slang: ran out of memory.")}; \
	} \
)

static void slang_load_spirv_code(
	slang::ISession *slang_session,
	slang::IModule *slang_module,
	char const *entry_point_name,
	Slang::ComPtr<slang::IBlob> &slang_spirv_code)
{
	using namespace Slang;
	using namespace slang;

	ComPtr<IEntryPoint> entry_point;
	SLANG_CHECK(slang_module->findEntryPointByName(entry_point_name, entry_point.writeRef()));

	std::array<IComponentType *, 2> component_types{
		slang_module,
		entry_point,
	};
	ComPtr<IComponentType> composed_program;
	SLANG_CHECK(slang_session->createCompositeComponentType(
		component_types.data(),
		component_types.size(),
		composed_program.writeRef()
	));

	ComPtr<IComponentType> linked_program;
	SLANG_CHECK(composed_program->link(linked_program.writeRef()));

	SLANG_CHECK(linked_program->getEntryPointCode(
		0, // entryPointIndex
		0, // targetIndex
		slang_spirv_code.writeRef()
	));
}

// TODO: Remove global variable.
static HINSTANCE win32_instance;

static void based_renderer_main();

int WINAPI WinMain(
	HINSTANCE instance,
	HINSTANCE prev_instance,
	LPSTR	 command_line,
	int	   show_command)
{
	UNUSED(prev_instance);
	UNUSED(command_line);
	UNUSED(show_command);

	win32_instance = instance;

	try
	{
		based_renderer_main();
	}
	catch (vk::OutOfHostMemoryError err)
	{
		win32_message_box(err.what(), "vk::OutOfHostMemoryError");
	}
	catch (vk::OutOfDeviceMemoryError err)
	{
		win32_message_box(err.what(), "vk::OutOfDeviceMemoryError");
	}
	catch (vk::InitializationFailedError err)
	{
		win32_message_box(err.what(), "vk::InitializationFailedError");
	}
	catch (vk::DeviceLostError err)
	{
		win32_message_box(err.what(), "vk::DeviceLostError");
	}
	catch (vk::MemoryMapFailedError err)
	{
		win32_message_box(err.what(), "vk::MemoryMapFailedError");
	}
	catch (vk::LayerNotPresentError err)
	{
		win32_message_box(err.what(), "vk::LayerNotPresentError");
	}
	catch (vk::ExtensionNotPresentError err)
	{
		win32_message_box(err.what(), "vk::ExtensionNotPresentError");
	}
	catch (vk::FeatureNotPresentError err)
	{
		win32_message_box(err.what(), "vk::FeatureNotPresentError");
	}
	catch (vk::IncompatibleDriverError err)
	{
		win32_message_box(err.what(), "vk::IncompatibleDriverError");
	}
	catch (vk::TooManyObjectsError err)
	{
		win32_message_box(err.what(), "vk::TooManyObjectsError");
	}
	catch (vk::FormatNotSupportedError err)
	{
		win32_message_box(err.what(), "vk::FormatNotSupportedError");
	}
	catch (vk::FragmentedPoolError err)
	{
		win32_message_box(err.what(), "vk::FragmentedPoolError");
	}
	catch (vk::UnknownError err)
	{
		win32_message_box(err.what(), "vk::UnknownError");
	}
	catch (vk::ValidationFailedError err)
	{
		win32_message_box(err.what(), "vk::ValidationFailedError");
	}
	catch (vk::OutOfPoolMemoryError err)
	{
		win32_message_box(err.what(), "vk::OutOfPoolMemoryError");
	}
	catch (vk::InvalidExternalHandleError err)
	{
		win32_message_box(err.what(), "vk::InvalidExternalHandleError");
	}
	catch (vk::InvalidOpaqueCaptureAddressError err)
	{
		win32_message_box(err.what(), "vk::InvalidOpaqueCaptureAddressError");
	}
	catch (vk::FragmentationError err)
	{
		win32_message_box(err.what(), "vk::FragmentationError");
	}
	catch (vk::NotPermittedError err)
	{
		win32_message_box(err.what(), "vk::NotPermittedError");
	}
	catch (vk::SurfaceLostKHRError err)
	{
		win32_message_box(err.what(), "vk::SurfaceLostKHRError");
	}
	catch (vk::NativeWindowInUseKHRError err)
	{
		win32_message_box(err.what(), "vk::NativeWindowInUseKHRError");
	}
	catch (vk::OutOfDateKHRError err)
	{
		win32_message_box(err.what(), "vk::OutOfDateKHRError");
	}
	catch (vk::InvalidShaderNVError err)
	{
		win32_message_box(err.what(), "vk::InvalidShaderNVError");
	}
	catch (vk::FullScreenExclusiveModeLostEXTError err)
	{
		win32_message_box(err.what(), "vk::FullScreenExclusiveModeLostEXTError");

		// NOTE: We are not actually using this
		// extension yet, but will be soon.
	}
	catch (vk::LogicError err)
	{
		win32_message_box(err.what(), "vk::LogicError");
	}
	catch (vk::SystemError err)
	{
		win32_message_box(err.what(), "vk::SystemError");
	}

	catch (std::invalid_argument err)
	{
		win32_message_box(err.what(), "std::invalid_argument");
	}
	catch (std::domain_error err)
	{
		win32_message_box(err.what(), "std::domain_error");
	}
	catch (std::length_error err)
	{
		win32_message_box(err.what(), "std::length_error");
	}
	catch (std::out_of_range err)
	{
		win32_message_box(err.what(), "std::out_of_range");
	}
	catch (std::range_error err)
	{
		win32_message_box(err.what(), "std::range_error");
	}
	catch (std::overflow_error err)
	{
		win32_message_box(err.what(), "std::overflow_error");
	}
	catch (std::underflow_error err)
	{
		win32_message_box(err.what(), "std::underflow_error");
	}
	catch (std::logic_error err)
	{
		win32_message_box(err.what(), "std::logic_error");
	}
	catch (std::runtime_error err)
	{
		win32_message_box(err.what(), "std::runtime_error");
	}
	catch (...)
	{
		win32_message_box("Failed for unknown reason.", "Error");
	}

	return 0;
}

static void based_renderer_main()
{
	vk::ApplicationInfo vulkan_app_info{
		"based_renderer",
		VK_API_VERSION_1_0,
		"based_renderer",
		VK_API_VERSION_1_0,
		VK_API_VERSION_1_4,
	};

	// TODO: Query instance layer support.

#if BASED_RENDERER_VULKAN_LAYERS
	std::vector<char const *> vulkan_instance_layers;
#if BASED_RENDERER_VULKAN_DEBUG
	vulkan_instance_layers.push_back("VK_LAYER_LUNARG_monitor");
#endif
#if BASED_RENDERER_VULKAN_VALIDATION
	vulkan_instance_layers.push_back("VK_LAYER_KHRONOS_validation");
	std::array<vk::ValidationFeatureEnableEXT, 2> vulkan_enabled_validation_features{
		vk::ValidationFeatureEnableEXT::eBestPractices,
		vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
	};
	vk::ValidationFeaturesEXT vulkan_validation_features{
		vulkan_enabled_validation_features,
	};
#endif
#endif // BASED_RENDERER_VULKAN_LAYERS

#if BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vk::DebugUtilsMessengerCreateInfoEXT vulkan_debug_output_info{
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

	// TODO: Query instance extension support.

	std::vector<char const *> vulkan_instance_extensions;
	vulkan_instance_extensions.push_back("VK_KHR_surface");
	vulkan_instance_extensions.push_back(VK_KHR_platform_surface);
#if BASED_RENDERER_VULKAN_LAYERS
	vulkan_instance_extensions.push_back("VK_EXT_layer_settings");
#endif
#if BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vulkan_instance_extensions.push_back("VK_EXT_debug_utils");
#endif

	vk::InstanceCreateInfo vulkan_instance_create_info{
		{},
		&vulkan_app_info,
#if BASED_RENDERER_VULKAN_LAYERS
		static_cast<uint32_t>(vulkan_instance_layers.size()),
		vulkan_instance_layers.data(),
#else
		0,
		nullptr,
#endif
		static_cast<uint32_t>(vulkan_instance_extensions.size()),
		vulkan_instance_extensions.data(),
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
		[](vk::PhysicalDevice p) 
		{
			vk::PhysicalDeviceProperties props = p.getProperties();
			return props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		}
	);

	auto vulkan_physical_device_properties = vulkan_physical_device.getProperties2<
		vk::PhysicalDeviceProperties2,
		vk::PhysicalDeviceVulkan11Properties,
		vk::PhysicalDeviceVulkan12Properties,
		vk::PhysicalDeviceVulkan13Properties,
		vk::PhysicalDeviceVulkan14Properties>();

	auto vulkan_physical_device_features = vulkan_physical_device.getFeatures2<
		vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan12Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceVulkan14Features>();

	std::vector<std::string> vulkan_missing_features;
	#define VULKAN_REQUIRE_FEATURE(FEATURE) STMT( \
		if (!features.FEATURE) \
		{ \
			vulkan_missing_features.push_back(STRINGIFY(FEATURE)); \
		} \
	)
	#define VULKAN_DISABLE_FEATURE(FEATURE) STMT( \
		features.FEATURE = vk::False; \
	)
	// You might say: this doesn't do anything. Why define it?
	// It's so that way, when scrolling through all of the Vulkan
	// features I might want to use, none of them are missing.
	#define VULKAN_ALLOW_FEATURE(FEATURE)

	// TODO: Should we say why we require or allow certain features? Right now it's pretty self-explanatory because there are so few, but eventually that might change.

	{
		auto &features = std::get<0>(vulkan_physical_device_features).features;
		VULKAN_ALLOW_FEATURE(robustBufferAccess);
		VULKAN_DISABLE_FEATURE(fullDrawIndexUint32);
		VULKAN_DISABLE_FEATURE(imageCubeArray);
		VULKAN_DISABLE_FEATURE(independentBlend);
		VULKAN_DISABLE_FEATURE(geometryShader);
		VULKAN_DISABLE_FEATURE(tessellationShader);
		VULKAN_DISABLE_FEATURE(sampleRateShading);
		VULKAN_DISABLE_FEATURE(dualSrcBlend);
		VULKAN_DISABLE_FEATURE(logicOp);
		VULKAN_DISABLE_FEATURE(multiDrawIndirect);
		VULKAN_DISABLE_FEATURE(drawIndirectFirstInstance);
		VULKAN_DISABLE_FEATURE(depthClamp);
		VULKAN_DISABLE_FEATURE(depthBiasClamp);
		VULKAN_DISABLE_FEATURE(fillModeNonSolid);
		VULKAN_DISABLE_FEATURE(depthBounds);
		VULKAN_DISABLE_FEATURE(wideLines);
		VULKAN_DISABLE_FEATURE(largePoints);
		VULKAN_DISABLE_FEATURE(alphaToOne);
		VULKAN_DISABLE_FEATURE(multiViewport);
		VULKAN_DISABLE_FEATURE(samplerAnisotropy);
		VULKAN_DISABLE_FEATURE(textureCompressionETC2);
		VULKAN_DISABLE_FEATURE(textureCompressionASTC_LDR);
		VULKAN_DISABLE_FEATURE(textureCompressionBC);
		VULKAN_DISABLE_FEATURE(occlusionQueryPrecise);
		VULKAN_DISABLE_FEATURE(pipelineStatisticsQuery);
		VULKAN_DISABLE_FEATURE(vertexPipelineStoresAndAtomics);
		VULKAN_DISABLE_FEATURE(fragmentStoresAndAtomics);
		VULKAN_DISABLE_FEATURE(shaderTessellationAndGeometryPointSize);
		VULKAN_DISABLE_FEATURE(shaderImageGatherExtended);
		VULKAN_DISABLE_FEATURE(shaderStorageImageExtendedFormats);
		VULKAN_DISABLE_FEATURE(shaderStorageImageMultisample);
		VULKAN_DISABLE_FEATURE(shaderStorageImageReadWithoutFormat);
		VULKAN_DISABLE_FEATURE(shaderStorageImageWriteWithoutFormat);
		VULKAN_DISABLE_FEATURE(shaderUniformBufferArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderSampledImageArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderStorageBufferArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderStorageImageArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderClipDistance);
		VULKAN_DISABLE_FEATURE(shaderCullDistance);
		VULKAN_DISABLE_FEATURE(shaderFloat64);
		VULKAN_DISABLE_FEATURE(shaderInt64);
		VULKAN_DISABLE_FEATURE(shaderInt16);
		VULKAN_DISABLE_FEATURE(shaderResourceResidency);
		VULKAN_DISABLE_FEATURE(shaderResourceMinLod);
		VULKAN_DISABLE_FEATURE(sparseBinding);
		VULKAN_DISABLE_FEATURE(sparseResidencyBuffer);
		VULKAN_DISABLE_FEATURE(sparseResidencyImage2D);
		VULKAN_DISABLE_FEATURE(sparseResidencyImage3D);
		VULKAN_DISABLE_FEATURE(sparseResidency2Samples);
		VULKAN_DISABLE_FEATURE(sparseResidency4Samples);
		VULKAN_DISABLE_FEATURE(sparseResidency8Samples);
		VULKAN_DISABLE_FEATURE(sparseResidency16Samples);
		VULKAN_DISABLE_FEATURE(sparseResidencyAliased);
		VULKAN_DISABLE_FEATURE(variableMultisampleRate);
		VULKAN_DISABLE_FEATURE(inheritedQueries);
	}
	{
		auto &features = std::get<1>(vulkan_physical_device_features);
		VULKAN_DISABLE_FEATURE(storageBuffer16BitAccess);
		VULKAN_DISABLE_FEATURE(uniformAndStorageBuffer16BitAccess);
		VULKAN_DISABLE_FEATURE(storagePushConstant16);
		VULKAN_DISABLE_FEATURE(storageInputOutput16);
		VULKAN_DISABLE_FEATURE(multiview);
		VULKAN_DISABLE_FEATURE(multiviewGeometryShader);
		VULKAN_DISABLE_FEATURE(multiviewTessellationShader);
		VULKAN_DISABLE_FEATURE(variablePointersStorageBuffer);
		VULKAN_DISABLE_FEATURE(variablePointers);
		VULKAN_DISABLE_FEATURE(protectedMemory);
		VULKAN_DISABLE_FEATURE(samplerYcbcrConversion);
		VULKAN_REQUIRE_FEATURE(shaderDrawParameters);
	}
	{
		auto &features = std::get<2>(vulkan_physical_device_features);
		VULKAN_DISABLE_FEATURE(samplerMirrorClampToEdge);
		VULKAN_DISABLE_FEATURE(drawIndirectCount);
		VULKAN_DISABLE_FEATURE(storageBuffer8BitAccess);
		VULKAN_DISABLE_FEATURE(uniformAndStorageBuffer8BitAccess);
		VULKAN_DISABLE_FEATURE(storagePushConstant8);
		VULKAN_DISABLE_FEATURE(shaderBufferInt64Atomics);
		VULKAN_DISABLE_FEATURE(shaderSharedInt64Atomics);
		VULKAN_DISABLE_FEATURE(shaderFloat16);
		VULKAN_DISABLE_FEATURE(shaderInt8);
		VULKAN_DISABLE_FEATURE(descriptorIndexing);
		VULKAN_DISABLE_FEATURE(shaderInputAttachmentArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderUniformTexelBufferArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderStorageTexelBufferArrayDynamicIndexing);
		VULKAN_DISABLE_FEATURE(shaderUniformBufferArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(shaderSampledImageArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(shaderStorageBufferArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(shaderStorageImageArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(shaderInputAttachmentArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(shaderUniformTexelBufferArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(shaderStorageTexelBufferArrayNonUniformIndexing);
		VULKAN_DISABLE_FEATURE(descriptorBindingUniformBufferUpdateAfterBind);
		VULKAN_DISABLE_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
		VULKAN_DISABLE_FEATURE(descriptorBindingStorageImageUpdateAfterBind);
		VULKAN_DISABLE_FEATURE(descriptorBindingStorageBufferUpdateAfterBind);
		VULKAN_DISABLE_FEATURE(descriptorBindingUniformTexelBufferUpdateAfterBind);
		VULKAN_DISABLE_FEATURE(descriptorBindingStorageTexelBufferUpdateAfterBind);
		VULKAN_DISABLE_FEATURE(descriptorBindingUpdateUnusedWhilePending);
		VULKAN_DISABLE_FEATURE(descriptorBindingPartiallyBound);
		VULKAN_DISABLE_FEATURE(descriptorBindingVariableDescriptorCount);
		VULKAN_DISABLE_FEATURE(runtimeDescriptorArray);
		VULKAN_DISABLE_FEATURE(samplerFilterMinmax);
		VULKAN_DISABLE_FEATURE(scalarBlockLayout);
		VULKAN_DISABLE_FEATURE(imagelessFramebuffer);
		VULKAN_DISABLE_FEATURE(uniformBufferStandardLayout);
		VULKAN_DISABLE_FEATURE(shaderSubgroupExtendedTypes);
		VULKAN_DISABLE_FEATURE(separateDepthStencilLayouts);
		VULKAN_DISABLE_FEATURE(hostQueryReset);
		VULKAN_DISABLE_FEATURE(timelineSemaphore);
		VULKAN_DISABLE_FEATURE(bufferDeviceAddress);
		VULKAN_DISABLE_FEATURE(bufferDeviceAddressCaptureReplay);
		VULKAN_DISABLE_FEATURE(bufferDeviceAddressMultiDevice);
		VULKAN_REQUIRE_FEATURE(vulkanMemoryModel); // TODO: Do we necessarily need these?
		VULKAN_REQUIRE_FEATURE(vulkanMemoryModelDeviceScope);
		VULKAN_REQUIRE_FEATURE(vulkanMemoryModelAvailabilityVisibilityChains);
		VULKAN_DISABLE_FEATURE(shaderOutputViewportIndex);
		VULKAN_DISABLE_FEATURE(shaderOutputLayer);
		VULKAN_DISABLE_FEATURE(subgroupBroadcastDynamicId);
	}
	{
		auto &features = std::get<3>(vulkan_physical_device_features);
		VULKAN_ALLOW_FEATURE(robustImageAccess);
		VULKAN_DISABLE_FEATURE(inlineUniformBlock);
		VULKAN_DISABLE_FEATURE(descriptorBindingInlineUniformBlockUpdateAfterBind);
		VULKAN_ALLOW_FEATURE(pipelineCreationCacheControl);
		VULKAN_DISABLE_FEATURE(privateData);
		VULKAN_DISABLE_FEATURE(shaderDemoteToHelperInvocation);
		VULKAN_DISABLE_FEATURE(shaderTerminateInvocation);
		VULKAN_DISABLE_FEATURE(subgroupSizeControl);
		VULKAN_DISABLE_FEATURE(computeFullSubgroups);
		VULKAN_REQUIRE_FEATURE(synchronization2);
		VULKAN_DISABLE_FEATURE(textureCompressionASTC_HDR);
		VULKAN_ALLOW_FEATURE(shaderZeroInitializeWorkgroupMemory);
		VULKAN_REQUIRE_FEATURE(dynamicRendering);
		VULKAN_DISABLE_FEATURE(shaderIntegerDotProduct);
		VULKAN_DISABLE_FEATURE(maintenance4);
	}
	{
		auto &features = std::get<4>(vulkan_physical_device_features);
		VULKAN_DISABLE_FEATURE(globalPriorityQuery);
		VULKAN_DISABLE_FEATURE(shaderSubgroupRotate);
		VULKAN_DISABLE_FEATURE(shaderSubgroupRotateClustered);
		VULKAN_DISABLE_FEATURE(shaderFloatControls2);
		VULKAN_DISABLE_FEATURE(shaderExpectAssume);
		VULKAN_DISABLE_FEATURE(rectangularLines);
		VULKAN_DISABLE_FEATURE(bresenhamLines);
		VULKAN_DISABLE_FEATURE(smoothLines);
		VULKAN_DISABLE_FEATURE(stippledRectangularLines);
		VULKAN_DISABLE_FEATURE(stippledBresenhamLines);
		VULKAN_DISABLE_FEATURE(stippledSmoothLines);
		VULKAN_DISABLE_FEATURE(vertexAttributeInstanceRateDivisor);
		VULKAN_DISABLE_FEATURE(vertexAttributeInstanceRateZeroDivisor);
		VULKAN_DISABLE_FEATURE(indexTypeUint8);
		VULKAN_DISABLE_FEATURE(dynamicRenderingLocalRead);
		VULKAN_DISABLE_FEATURE(maintenance5);
		VULKAN_DISABLE_FEATURE(maintenance6);
		VULKAN_DISABLE_FEATURE(pipelineProtectedAccess);
		VULKAN_DISABLE_FEATURE(pipelineRobustness);
		VULKAN_DISABLE_FEATURE(hostImageCopy);
		VULKAN_DISABLE_FEATURE(pushDescriptor);
	}

	if (vulkan_missing_features.size() > 0)
	{
		throw vk::FeatureNotPresentError{to_string(vulkan_missing_features)};
	}

	std::vector<vk::QueueFamilyProperties> vulkan_queue_family_properties = vulkan_physical_device.getQueueFamilyProperties();

	// TODO: This is stupid. Find out how queue priorities should be done.
	std::array<float, 64> vulkan_queue_priorities{
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

	// TODO: Query device extension support.

	std::vector<char const *> vulkan_device_extensions;
	vulkan_device_extensions.push_back("VK_KHR_swapchain");

	vk::Device vulkan_device = vulkan_physical_device.createDevice(vk::DeviceCreateInfo{
		{}, 
		vulkan_device_queue_infos,
		{},
		vulkan_device_extensions,
		{},
		&std::get<0>(vulkan_physical_device_features),
	});

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

	// Why use C for loops here? Why not std::find_if? I tried that, and it came out uglier and harder to understand. However, in other cases, like selecting which physical device to use, std::find_if is actually pretty convenient.

	std::optional<size_t> vulkan_graphics_queue_family_idx;
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if ((vulkan_queue_family_properties[i].queueFlags & vk::QueueFlags{vk::QueueFlagBits::eGraphics}) != vk::QueueFlags{})
		{
			vulkan_graphics_queue_family_idx = i;
			break;
		}
	}
	vk::Queue vulkan_graphics_queue = vulkan_queues[vulkan_graphics_queue_family_idx.value()][0];

	std::optional<size_t> vulkan_transfer_queue_family_idx;
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if ((vulkan_queue_family_properties[i].queueFlags & vk::QueueFlags{vk::QueueFlagBits::eTransfer}) != vk::QueueFlags{})
		{
			vulkan_transfer_queue_family_idx = i;
			break;
		}
	}
	vk::Queue vulkan_transfer_queue = vulkan_queues[vulkan_transfer_queue_family_idx.value()][0];

	vk::CommandPool vulkan_graphics_command_pool = vulkan_device.createCommandPool({
		vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
		static_cast<uint32_t>(vulkan_graphics_queue_family_idx.value()),
	});

	vk::CommandPool vulkan_transfer_command_pool;
	if (vulkan_graphics_queue_family_idx != vulkan_transfer_queue_family_idx)
	{
		vulkan_transfer_command_pool = vulkan_device.createCommandPool({
			vk::CommandPoolCreateFlags(),
			static_cast<uint32_t>(vulkan_transfer_queue_family_idx.value()),
		});
	}
	else
	{
		vulkan_transfer_command_pool = vulkan_graphics_command_pool;
	}

	HMONITOR win32_monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitor_info {sizeof(MONITORINFO)};
	if (!GetMonitorInfoW(win32_monitor, &monitor_info)) 
	{
		throw win32_system_error();
	}
	int32_t monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
	int32_t monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

	WNDCLASSEXW win32_window_class{
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
		throw win32_system_error();
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
		throw win32_system_error();
	}

	RECT win32_client_rect;
	GetClientRect(win32_window, &win32_client_rect);
	int32_t client_width = win32_client_rect.right - win32_client_rect.left;
	int32_t client_height = win32_client_rect.bottom - win32_client_rect.top;

	vk::SurfaceKHR vulkan_surface = vulkan_instance.createWin32SurfaceKHR({
		{},
		win32_instance,
		win32_window,
	});

	std::optional<size_t> vulkan_present_queue_family_idx;
	for (size_t i = 0; i < vulkan_queue_family_properties.size(); ++i)
	{
		if (vulkan_physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), vulkan_surface))
		{
			vulkan_present_queue_family_idx = i;
			break;
		}
	}
	vk::Queue vulkan_present_queue = vulkan_queues[vulkan_present_queue_family_idx.value()][0];

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

	vk::SurfaceTransformFlagBitsKHR vulkan_pre_transform = 
		(vulkan_surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) 
		? vk::SurfaceTransformFlagBitsKHR::eIdentity 
		: vulkan_surface_capabilities.currentTransform;

	vk::CompositeAlphaFlagBitsKHR vulkan_composite_alpha = (vulkan_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied : (vulkan_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied : (vulkan_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

	vk::SwapchainCreateInfoKHR vulkan_swapchain_create_info{
		vk::SwapchainCreateFlagsKHR(),
		vulkan_surface,
		// TODO: Right now, you are still basically assuming that the image count will be 2.
		// IIRC, having an image count higher than two actually complicates synchronization somewhat.
		// I might be wrong though. In any case, it's worth looking into.
		std::clamp(2u, vulkan_surface_capabilities.minImageCount, vulkan_surface_capabilities.maxImageCount),
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

	std::array<uint32_t, 2> vulkan_queue_family_indices{
		static_cast<uint32_t>(vulkan_graphics_queue_family_idx.value()),
		static_cast<uint32_t>(vulkan_transfer_queue_family_idx.value())
	};
	if (vulkan_graphics_queue_family_idx != vulkan_transfer_queue_family_idx)
	{
		// TODO: Is this really necessary? I would have thought that using transfer queue operations is enough.
		vulkan_swapchain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
		vulkan_swapchain_create_info.queueFamilyIndexCount = static_cast<uint32_t>(vulkan_queue_family_indices.size());
		vulkan_swapchain_create_info.pQueueFamilyIndices = vulkan_queue_family_indices.data();
	}

	vk::SwapchainKHR vulkan_swapchain = vulkan_device.createSwapchainKHR(vulkan_swapchain_create_info);

	std::vector<vk::Image> vulkan_swapchain_images = vulkan_device.getSwapchainImagesKHR(vulkan_swapchain);

	std::vector<vk::ImageView> vulkan_swapchain_image_views;
	vulkan_swapchain_image_views.reserve(vulkan_swapchain_images.size());
	vk::ImageViewCreateInfo vulkan_image_view_create_info{
		{}, 
		{},
		vk::ImageViewType::e2D, 
		vulkan_format, 
		{}, 
		{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	for (vk::Image image : vulkan_swapchain_images)
	{
		vulkan_image_view_create_info.image = image;
		vulkan_swapchain_image_views.push_back(vulkan_device.createImageView(vulkan_image_view_create_info));
	}

	std::vector<vk::CommandBuffer> vulkan_graphics_command_buffers = vulkan_device.allocateCommandBuffers({
		vulkan_graphics_command_pool, 
		vk::CommandBufferLevel::ePrimary, 
		static_cast<uint32_t>(vulkan_swapchain_images.size()),
	});

	vk::CommandBuffer vulkan_transfer_command_buffer;
	if (vulkan_graphics_command_pool != vulkan_transfer_command_pool)
	{
		std::vector<vk::CommandBuffer> v = vulkan_device.allocateCommandBuffers({
			vulkan_transfer_command_pool, 
			vk::CommandBufferLevel::ePrimary, 
			1,
		});
		vulkan_transfer_command_buffer = v[0];
	}
	else
	{
		// Just don't use the transfer command buffer then!
	}

	std::vector<vk::Fence> vulkan_fences{vulkan_swapchain_images.size()};
	for (vk::Fence &fence : vulkan_fences) 
	{
		fence = vulkan_device.createFence({{vk::FenceCreateFlagBits::eSignaled}});
	}

	std::vector<vk::Semaphore> vulkan_semaphores_wait{vulkan_swapchain_images.size()};
	std::vector<vk::Semaphore> vulkan_semaphores_signal{vulkan_swapchain_images.size()};
	for (size_t i = 0; i < vulkan_swapchain_images.size(); ++i)
	{
		vulkan_semaphores_wait[i] = vulkan_device.createSemaphore({});
		vulkan_semaphores_signal[i] = vulkan_device.createSemaphore({});
	}

	// slang_init
	Slang::ComPtr<slang::IGlobalSession> slang_global_session;
	Slang::ComPtr<slang::ISession> slang_session;
	{
		using namespace Slang;
		using namespace slang;

		SlangGlobalSessionDesc global_session_desc{};
		SLANG_CHECK(createGlobalSession(&global_session_desc, slang_global_session.writeRef()));

		TargetDesc target_desc{
			.format = SLANG_SPIRV,
			.profile = slang_global_session->findProfile("glsl_450"),
			.compilerOptionEntries = nullptr,
			.compilerOptionEntryCount = 0,
		};

		std::array<char const*, 1> const search_paths{
			"src",
		};

		SessionDesc session_desc{
			.targets = &target_desc,
			.targetCount = 1,
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.searchPaths = search_paths.data(),
			.searchPathCount = search_paths.size(),
			.preprocessorMacros = nullptr,
			.preprocessorMacroCount = 0,
			.enableEffectAnnotations = false,
			.compilerOptionEntries = nullptr,
			.compilerOptionEntryCount = 0,
#if BASED_RENDERER_SLANG_SPIRV_VALIDATION
			.skipSPIRVValidation = true,
#endif
		};
		SLANG_CHECK(slang_global_session->createSession(session_desc, slang_session.writeRef()));
	}

	vk::PipelineCacheCreateFlagBits vulkan_pipeline_cache_flag_bits{};
	if (std::get<3>(vulkan_physical_device_features).pipelineCreationCacheControl)
	{
		vulkan_pipeline_cache_flag_bits = vk::PipelineCacheCreateFlagBits::eExternallySynchronized;
	}
	vk::PipelineCache vulkan_pipeline_cache = vulkan_device.createPipelineCache(
		{vulkan_pipeline_cache_flag_bits}
	);

	vk::PipelineLayout vulkan_pipeline_layout = vulkan_device.createPipelineLayout({});

	Slang::ComPtr<slang::IModule> slang_module;
	Slang::ComPtr<slang::IBlob> slang_module_diagnostics;
	slang_module = slang_session->loadModule("src/shader", slang_module_diagnostics.writeRef());
	if (slang_module_diagnostics.get())
	{
		// TODO: Find a way to get shader compile errors in the Sublime Text console.
		throw std::runtime_error{
			std::string{std::string_view{
				static_cast<char const *>(slang_module_diagnostics->getBufferPointer()),
				static_cast<size_t>(slang_module_diagnostics->getBufferSize())
			}}
		};
	}

	Slang::ComPtr<slang::IBlob> slang_spirv_code_vs;
	slang_load_spirv_code(slang_session, slang_module, "vs", slang_spirv_code_vs);
	vk::ShaderModule vulkan_vertex_shader_module = vulkan_device.createShaderModule({
		{},
		static_cast<uint32_t>(slang_spirv_code_vs->getBufferSize()),
		static_cast<uint32_t const *>(slang_spirv_code_vs->getBufferPointer()),
	});
	vk::PipelineShaderStageCreateInfo vulkan_vertex_shader_stage_create_info{
		{},
		vk::ShaderStageFlagBits::eVertex,
		vulkan_vertex_shader_module,
		"main",
	};

	Slang::ComPtr<slang::IBlob> slang_spirv_code_ps;
	slang_load_spirv_code(slang_session, slang_module, "ps", slang_spirv_code_ps);
	vk::ShaderModule vulkan_fragment_shader_module = vulkan_device.createShaderModule({
		{},
		static_cast<uint32_t>(slang_spirv_code_ps->getBufferSize()),
		static_cast<uint32_t const *>(slang_spirv_code_ps->getBufferPointer()),
	});
	vk::PipelineShaderStageCreateInfo vulkan_fragment_shader_stage_create_info{
		{},
		vk::ShaderStageFlagBits::eFragment,
		vulkan_fragment_shader_module,
		"main",
	};

	std::array<vk::PipelineShaderStageCreateInfo, 2> vulkan_shader_stage_create_infos{
		vulkan_vertex_shader_stage_create_info,
		vulkan_fragment_shader_stage_create_info,
	};

	// std::array<vk::VertexInputBindingDescription, 1> vulkan_vertex_input_binding_descriptions{

	// };

	// std::array<vk::VertexInputAttributeDescription, 1> vulkan_vertex_input_attribute_descriptions{

	// };

	vk::PipelineVertexInputStateCreateInfo vulkan_vertex_input_state_create_info{
		vk::PipelineVertexInputStateCreateFlags{},
		// vulkan_vertex_input_binding_descriptions,
		// vulkan_vertex_input_attribute_descriptions,
	};

	vk::PipelineInputAssemblyStateCreateInfo vulkan_pipeline_input_assembly_state_create_info{
		{},
		vk::PrimitiveTopology::eTriangleList,
	};

	std::array<vk::Viewport, 1> vulkan_viewports{
		vk::Viewport{
			0.0f,
			0.0f,
			static_cast<float>(client_width),
			static_cast<float>(client_height),
			0.0f,
			1.0f,
		},
	};

	std::array<vk::Rect2D, 1> vulkan_scissors{
		vk::Rect2D{
			vk::Offset2D{0, 0},
			vk::Extent2D{
				static_cast<uint32_t>(client_width), 
				static_cast<uint32_t>(client_height)
			},
		},
	};

	vk::PipelineViewportStateCreateInfo vulkan_pipeline_viewport_state_create_info{
		vk::PipelineViewportStateCreateFlags{},
		vulkan_viewports,
		vulkan_scissors,
	};

	vk::PipelineRasterizationStateCreateInfo vulkan_pipeline_rasterization_state_create_info{
		{},
		{},
		{},
		vk::PolygonMode::eFill,
		{},
		vk::FrontFace::eCounterClockwise,
		{},
		{},
		{},
		{},
		1.0f,
	};
	vk::PipelineMultisampleStateCreateInfo vulkan_pipeline_multisample_state_create_info{};
	vk::PipelineDepthStencilStateCreateInfo vulkan_pipeline_depth_stencil_state_create_info{};

	std::array<vk::PipelineColorBlendAttachmentState, 1> vulkan_pipeline_color_blend_attachment_states{
		vk::PipelineColorBlendAttachmentState{},
	};

	vk::PipelineColorBlendStateCreateInfo vulkan_pipeline_color_blend_state_create_info{
		vk::PipelineColorBlendStateCreateFlags{},
		{},
		vk::LogicOp::eClear,
		vulkan_pipeline_color_blend_attachment_states,
	};

	vk::PipelineDynamicStateCreateInfo vulkan_pipeline_dynamic_state_create_info{};

	std::array<vk::Format const, 1> const vulkan_pipeline_rendering_formats{
		vulkan_format,
	};

	vk::PipelineRenderingCreateInfo vulkan_pipeline_rendering_create_info{
		0,
		vulkan_pipeline_rendering_formats,
	};

	vk::GraphicsPipelineCreateInfo vulkan_graphics_pipeline_create_info{
#if BASED_RENDERER_VULKAN_DISABLE_PIPELINE_OPTIMIZATION
		vk::PipelineCreateFlagBits::eDisableOptimization,
#else
		{},
#endif
		vulkan_shader_stage_create_infos,
		&vulkan_vertex_input_state_create_info,
		&vulkan_pipeline_input_assembly_state_create_info,
		nullptr,
		&vulkan_pipeline_viewport_state_create_info,
		&vulkan_pipeline_rasterization_state_create_info,
		&vulkan_pipeline_multisample_state_create_info,
		&vulkan_pipeline_depth_stencil_state_create_info,
		&vulkan_pipeline_color_blend_state_create_info,
		&vulkan_pipeline_dynamic_state_create_info,
		vulkan_pipeline_layout,
		{},
		{},
		{},
		{},
		&vulkan_pipeline_rendering_create_info,
	};


	auto vulkan_pipelines = *vulkan_device.createGraphicsPipelines(
		vulkan_pipeline_cache,
		{
			vulkan_graphics_pipeline_create_info
		}
	);

	size_t vulkan_frame_idx = 0;

	win32_running = true;
	while (win32_running) 
	{
		MSG win32_message;
		if (PeekMessageW(&win32_message, win32_window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&win32_message);
			DispatchMessageW(&win32_message);
			continue;
		}
		
		vk::detail::resultCheck(vulkan_device.waitForFences(
			{vulkan_fences[vulkan_frame_idx]}, 
			vk::True, 
			std::numeric_limits<uint64_t>::max()), "Failed to wait for fence.");
		vulkan_device.resetFences({vulkan_fences[vulkan_frame_idx]});

		uint32_t vulkan_image_idx = *vulkan_device.acquireNextImageKHR(vulkan_swapchain, std::numeric_limits<uint64_t>::max(), vulkan_semaphores_wait[vulkan_frame_idx]);

		// We only show the window once we've arrived back at the first frame.
		// This only makes sense if there are just two frames, that is, one backbuffer
		// and one frontbuffer. Which is to say, it is to be thrown away!
		if (vulkan_image_idx == 0) 
		{
			static int win32_window_ready = -1;
			if (win32_window_ready == -1) 
			{
				win32_window_ready += 1;
			} 
			else if (win32_window_ready == 0) 
			{
				win32_window_ready += 1;
				ShowWindow(win32_window, SW_SHOW);
			}
		}

		vk::CommandBuffer cb = vulkan_graphics_command_buffers[vulkan_frame_idx];
		cb.begin({
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		});

		static size_t staged = 0;

		std::array<vk::ImageMemoryBarrier2, 1> vulkan_image_memory_barriers_render{
			vk::ImageMemoryBarrier2{
				vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
				vk::AccessFlags2{},
				vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
				vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eColorAttachmentOptimal,
				0, // TODO
				0, // TODO
				vulkan_swapchain_images[vulkan_image_idx],
				vk::ImageSubresourceRange{
					vk::ImageAspectFlags{vk::ImageAspectFlagBits::eColor},
					0,
					1,
					0,
					1,
				},
			},
		};

		if (staged == vulkan_swapchain_images.size())
		{
			vulkan_image_memory_barriers_render[0].oldLayout = vk::ImageLayout::ePresentSrcKHR;
		}

		cb.pipelineBarrier2({
			vk::DependencyFlags{},
			{},
			{},
			vulkan_image_memory_barriers_render,
		});

		std::array<vk::RenderingAttachmentInfo, 1> vulkan_rendering_attachment_infos{
			vk::RenderingAttachmentInfo{
				vulkan_swapchain_image_views[vulkan_image_idx],
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::ResolveModeFlagBits::eNone,
				vk::ImageView{},
				vk::ImageLayout::eUndefined,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::ClearValue{},
			},
		};

		cb.beginRendering({
			vk::RenderingFlags{},
			vk::Rect2D{
				vk::Offset2D{0, 0},
				vk::Extent2D{static_cast<uint32_t>(client_width), static_cast<uint32_t>(client_height)},
			},
			1,
			0,
			vulkan_rendering_attachment_infos,
		});

		cb.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			vulkan_pipelines[0]
		);
		cb.draw(6, 1, 0, 0);

		cb.endRendering();

		std::array<vk::ImageMemoryBarrier2, 1> vulkan_image_memory_barriers_present{
			vk::ImageMemoryBarrier2{
				vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
				vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
				vk::PipelineStageFlags2{},
				vk::AccessFlags2{},
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::ePresentSrcKHR,
				0, // TODO
				0, // TODO
				vulkan_swapchain_images[vulkan_image_idx],
				vk::ImageSubresourceRange{
					vk::ImageAspectFlags{vk::ImageAspectFlagBits::eColor},
					0,
					1,
					0,
					1,
				},
			},
		};

		cb.pipelineBarrier2({
			vk::DependencyFlags{},
			{},
			{},
			vulkan_image_memory_barriers_present,
		});
		if (staged < vulkan_swapchain_images.size())
		{
			staged += 1;
		}

		cb.end();

		std::array<vk::SemaphoreSubmitInfo, 1> vulkan_wait_semaphore_infos{
			vk::SemaphoreSubmitInfo{
				vulkan_semaphores_wait[vulkan_frame_idx],
				0,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			},
		};

		std::array<vk::CommandBufferSubmitInfo, 1> vulkan_command_buffer_submit_infos{
			{
				cb,
			},
		};

		std::array<vk::SemaphoreSubmitInfo, 1> vulkan_signal_semaphore_infos{
			vk::SemaphoreSubmitInfo{
				vulkan_semaphores_signal[vulkan_frame_idx],
				0,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			},
		};

		std::array<vk::SubmitInfo2, 1> vulkan_submit_infos{
			vk::SubmitInfo2{
				{},
				vulkan_wait_semaphore_infos,
				vulkan_command_buffer_submit_infos,
				vulkan_signal_semaphore_infos,
			}
		};
		vulkan_graphics_queue.submit2(vulkan_submit_infos, vulkan_fences[vulkan_frame_idx]);

		std::array<vk::Semaphore, 1> vulkan_signal_semaphores{vulkan_semaphores_signal[vulkan_frame_idx]};
		std::array<vk::SwapchainKHR, 1> vulkan_swapchains{vulkan_swapchain};
		std::array<uint32_t, 1> vulkan_image_indices{vulkan_image_idx};
		std::array<vk::Result, 1> vulkan_present_results;
		// TODO: Use the present queue.
		vk::detail::resultCheck(vulkan_graphics_queue.presentKHR({
			vulkan_signal_semaphores,
			vulkan_swapchains,
			vulkan_image_indices,
			vulkan_present_results
		}), "Failed to present.");
		vk::detail::resultCheck(vulkan_present_results[0], "Failed to present.");

		vulkan_frame_idx = (vulkan_frame_idx + 1) % vulkan_swapchain_images.size();
	}
}