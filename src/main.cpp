#include "pch.hpp"

#define UNUSED(X) (void)(X)

#define BASED_RENDERER_DEBUG 1

#define BASED_RENDERER_VULKAN_DEBUG BASED_RENDERER_DEBUG
#define BASED_RENDERER_VULKAN_VALIDATION BASED_RENDERER_VULKAN_DEBUG
#define BASED_RENDERER_VULKAN_DEBUG_OUTPUT BASED_RENDERER_VULKAN_DEBUG
#define BASED_RENDERER_VULKAN_LAYERS (BASED_RENDERER_VULKAN_DEBUG || BASED_RENDERER_VULKAN_VALIDATION)

#define BASED_RENDERER_FULLSCREEN 0

#define VK_KHR_platform_surface "VK_KHR_win32_surface"

// Works just like std::print, except it prints to the debug console.
template<class... Args> void 
static dprint(std::format_string<Args...> fmt, Args&&... args) 
{
	std::string s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringA(s.c_str());
}

// Same, but the format string is a wide string.
template<class... Args> void 
static dprint(std::wformat_string<Args...> fmt, Args&&... args) 
{
	std::wstring s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringW(s.c_str());
}

// A clever way I found to remove an element from an std::vector.
// Assumes that i is within the bounds of v.
template <class T>
static void unordered_remove(std::vector<T> &v, size_t const i) 
{
	v[i] = v.back();
	v.pop_back();
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

vk::Bool32 VKAPI_PTR vulkan_debug_callback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
	vk::DebugUtilsMessageTypeFlagsEXT message_types,
	vk::DebugUtilsMessengerCallbackDataEXT const *callback_data,
	void *user_data)
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
	// preferred ones. If that doesn't work, we throw an exception (TODO).

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

	// TODO: Throw exception if memoryTypeIndex is still not set.
	return std::numeric_limits<uint32_t>::max();
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
//    staging buffer should be host visible.
// 3. Now you can call vulkan_allocate. When passing the buffers and images,
//    you should only fill out the fields under the "in" comment.
// 4. For each buffer and image, the "out" fields will be filled. In all
//    likelihood, you will only ever need to use the "memory" and "offset"
//    fields, but the others are there as well just in case.
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

			buffer_allocation.memory = device.allocateMemory(memory_allocate_info);;

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
		auto IsPowerOf2 = [](vk::DeviceSize x) 
		{
			return (x & (x-1)) == 0;
		};
		auto AlignForward = [IsPowerOf2](vk::DeviceSize offset, vk::DeviceSize align) 
		{
			// TODO: Switch to using an exception.
			assert(IsPowerOf2(align));

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

		for (VulkanBufferAllocation &buffer_allocation : buffer_allocations) 
		{
			if (buffer_allocation.memory_type_idx == memory_type_idx && !buffer_allocation.memory) 
			{
				memory_offset = AlignForward(memory_offset, buffer_allocation.align);

				vk::BindBufferMemoryInfo bind_buffer_memory_info;
				bind_buffer_memory_info.buffer = buffer_allocation.buffer;
				bind_buffer_memory_info.memoryOffset = memory_offset;
				bind_buffer_memory_infos.push_back(bind_buffer_memory_info);

				memory_offset += buffer_allocation.size;
			}
		}

		for (VulkanImageAllocation &image_allocation : image_allocations) 
		{
			if (image_allocation.memory_type_idx == memory_type_idx && !image_allocation.memory) 
			{
				memory_offset = AlignForward(memory_offset, image_allocation.align);

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

int WINAPI WinMain(
	HINSTANCE win32_instance,
	HINSTANCE win32_prev_instance,
	LPSTR     win32_command_line,
	int       win32_show_command) 
{
	UNUSED(win32_prev_instance);
	UNUSED(win32_command_line);
	UNUSED(win32_show_command);

	vk::ApplicationInfo vulkan_app_info{
		"based_renderer",
		VK_API_VERSION_1_0,
		"based_renderer",
		VK_API_VERSION_1_0,
		VK_API_VERSION_1_4,
	};

#if BASED_RENDERER_VULKAN_LAYERS
	std::vector<char const *> vulkan_layers;
#if BASED_RENDERER_VULKAN_DEBUG
	vulkan_layers.push_back("VK_LAYER_LUNARG_monitor");
#endif
#if BASED_RENDERER_VULKAN_VALIDATION
	vulkan_layers.push_back("VK_LAYER_KHRONOS_validation");
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

	std::vector<char const *> vulkan_extensions;
	vulkan_extensions.push_back("VK_KHR_surface");
	vulkan_extensions.push_back(VK_KHR_platform_surface);
#if BASED_RENDERER_VULKAN_LAYERS
	vulkan_extensions.push_back("VK_EXT_layer_settings");
#endif
#if BASED_RENDERER_VULKAN_DEBUG_OUTPUT
	vulkan_extensions.push_back("VK_EXT_debug_utils");
#endif

	vk::InstanceCreateInfo vulkan_instance_create_info{
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

	{
		auto &features = std::get<0>(vulkan_physical_device_features).features;
		features.robustBufferAccess = vk::False;
		features.fullDrawIndexUint32 = vk::False;
		features.imageCubeArray = vk::False;
		features.independentBlend = vk::False;
		features.geometryShader = vk::False;
		features.tessellationShader = vk::False;
		features.sampleRateShading = vk::False;
		features.dualSrcBlend = vk::False;
		features.logicOp = vk::False;
		features.multiDrawIndirect = vk::False;
		features.drawIndirectFirstInstance = vk::False;
		features.depthClamp = vk::False;
		features.depthBiasClamp = vk::False;
		features.fillModeNonSolid = vk::False;
		features.depthBounds = vk::False;
		features.wideLines = vk::False;
		features.largePoints = vk::False;
		features.alphaToOne = vk::False;
		features.multiViewport = vk::False;
		features.samplerAnisotropy = vk::False;
		features.textureCompressionETC2 = vk::False;
		features.textureCompressionASTC_LDR = vk::False;
		features.textureCompressionBC = vk::False;
		features.occlusionQueryPrecise = vk::False;
		features.pipelineStatisticsQuery = vk::False;
		features.vertexPipelineStoresAndAtomics = vk::False;
		features.fragmentStoresAndAtomics = vk::False;
		features.shaderTessellationAndGeometryPointSize = vk::False;
		features.shaderImageGatherExtended = vk::False;
		features.shaderStorageImageExtendedFormats = vk::False;
		features.shaderStorageImageMultisample = vk::False;
		features.shaderStorageImageReadWithoutFormat = vk::False;
		features.shaderStorageImageWriteWithoutFormat = vk::False;
		features.shaderUniformBufferArrayDynamicIndexing = vk::False;
		features.shaderSampledImageArrayDynamicIndexing = vk::False;
		features.shaderStorageBufferArrayDynamicIndexing = vk::False;
		features.shaderStorageImageArrayDynamicIndexing = vk::False;
		features.shaderClipDistance = vk::False;
		features.shaderCullDistance = vk::False;
		features.shaderFloat64 = vk::False;
		features.shaderInt64 = vk::False;
		features.shaderInt16 = vk::False;
		features.shaderResourceResidency = vk::False;
		features.shaderResourceMinLod = vk::False;
		features.sparseBinding = vk::False;
		features.sparseResidencyBuffer = vk::False;
		features.sparseResidencyImage2D = vk::False;
		features.sparseResidencyImage3D = vk::False;
		features.sparseResidency2Samples = vk::False;
		features.sparseResidency4Samples = vk::False;
		features.sparseResidency8Samples = vk::False;
		features.sparseResidency16Samples = vk::False;
		features.sparseResidencyAliased = vk::False;
		features.variableMultisampleRate = vk::False;
		features.inheritedQueries = vk::False;
	}
	{
		auto &features = std::get<1>(vulkan_physical_device_features);
		features.storageBuffer16BitAccess = vk::False;
		features.uniformAndStorageBuffer16BitAccess = vk::False;
		features.storagePushConstant16 = vk::False;
		features.storageInputOutput16 = vk::False;
		features.multiview = vk::False;
		features.multiviewGeometryShader = vk::False;
		features.multiviewTessellationShader = vk::False;
		features.variablePointersStorageBuffer = vk::False;
		features.variablePointers = vk::False;
		features.protectedMemory = vk::False;
		features.samplerYcbcrConversion = vk::False;
		features.shaderDrawParameters = vk::False;
	}
	{
		auto &features = std::get<2>(vulkan_physical_device_features);
		features.samplerMirrorClampToEdge = vk::False;
		features.drawIndirectCount = vk::False;
		features.storageBuffer8BitAccess = vk::False;
		features.uniformAndStorageBuffer8BitAccess = vk::False;
		features.storagePushConstant8 = vk::False;
		features.shaderBufferInt64Atomics = vk::False;
		features.shaderSharedInt64Atomics = vk::False;
		features.shaderFloat16 = vk::False;
		features.shaderInt8 = vk::False;
		features.descriptorIndexing = vk::False;
		features.shaderInputAttachmentArrayDynamicIndexing = vk::False;
		features.shaderUniformTexelBufferArrayDynamicIndexing = vk::False;
		features.shaderStorageTexelBufferArrayDynamicIndexing = vk::False;
		features.shaderUniformBufferArrayNonUniformIndexing = vk::False;
		features.shaderSampledImageArrayNonUniformIndexing = vk::False;
		features.shaderStorageBufferArrayNonUniformIndexing = vk::False;
		features.shaderStorageImageArrayNonUniformIndexing = vk::False;
		features.shaderInputAttachmentArrayNonUniformIndexing = vk::False;
		features.shaderUniformTexelBufferArrayNonUniformIndexing = vk::False;
		features.shaderStorageTexelBufferArrayNonUniformIndexing = vk::False;
		features.descriptorBindingUniformBufferUpdateAfterBind = vk::False;
		features.descriptorBindingSampledImageUpdateAfterBind = vk::False;
		features.descriptorBindingStorageImageUpdateAfterBind = vk::False;
		features.descriptorBindingStorageBufferUpdateAfterBind = vk::False;
		features.descriptorBindingUniformTexelBufferUpdateAfterBind = vk::False;
		features.descriptorBindingStorageTexelBufferUpdateAfterBind = vk::False;
		features.descriptorBindingUpdateUnusedWhilePending = vk::False;
		features.descriptorBindingPartiallyBound = vk::False;
		features.descriptorBindingVariableDescriptorCount = vk::False;
		features.runtimeDescriptorArray = vk::False;
		features.samplerFilterMinmax = vk::False;
		features.scalarBlockLayout = vk::False;
		features.imagelessFramebuffer = vk::False;
		features.uniformBufferStandardLayout = vk::False;
		features.shaderSubgroupExtendedTypes = vk::False;
		features.separateDepthStencilLayouts = vk::False;
		features.hostQueryReset = vk::False;
		features.timelineSemaphore = vk::False;
		features.bufferDeviceAddress = vk::False;
		features.bufferDeviceAddressCaptureReplay = vk::False;
		features.bufferDeviceAddressMultiDevice = vk::False;
		features.vulkanMemoryModel = vk::False;
		features.vulkanMemoryModelDeviceScope = vk::False;
		features.vulkanMemoryModelAvailabilityVisibilityChains = vk::False;
		features.shaderOutputViewportIndex = vk::False;
		features.shaderOutputLayer = vk::False;
		features.subgroupBroadcastDynamicId = vk::False;
	}
	{
		auto &features = std::get<3>(vulkan_physical_device_features);
		features.robustImageAccess = vk::False;
		features.inlineUniformBlock = vk::False;
		features.descriptorBindingInlineUniformBlockUpdateAfterBind = vk::False;
		features.pipelineCreationCacheControl = vk::False;
		features.privateData = vk::False;
		features.shaderDemoteToHelperInvocation = vk::False;
		features.shaderTerminateInvocation = vk::False;
		features.subgroupSizeControl = vk::False;
		features.computeFullSubgroups = vk::False;
		features.synchronization2 = vk::False;
		features.textureCompressionASTC_HDR = vk::False;
		features.shaderZeroInitializeWorkgroupMemory = vk::False;
		features.dynamicRendering = vk::False;
		features.shaderIntegerDotProduct = vk::False;
		features.maintenance4 = vk::False;
	}
	{
		auto &features = std::get<4>(vulkan_physical_device_features);
		features.globalPriorityQuery = vk::False;
		features.shaderSubgroupRotate = vk::False;
		features.shaderSubgroupRotateClustered = vk::False;
		features.shaderFloatControls2 = vk::False;
		features.shaderExpectAssume = vk::False;
		features.rectangularLines = vk::False;
		features.bresenhamLines = vk::False;
		features.smoothLines = vk::False;
		features.stippledRectangularLines = vk::False;
		features.stippledBresenhamLines = vk::False;
		features.stippledSmoothLines = vk::False;
		features.vertexAttributeInstanceRateDivisor = vk::False;
		features.vertexAttributeInstanceRateZeroDivisor = vk::False;
		features.indexTypeUint8 = vk::False;
		features.dynamicRenderingLocalRead = vk::False;
		features.maintenance5 = vk::False;
		features.maintenance6 = vk::False;
		features.pipelineProtectedAccess = vk::False;
		features.pipelineRobustness = vk::False;
		features.hostImageCopy = vk::False;
		features.pushDescriptor = vk::False;
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
	    return -3;
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

	std::vector<vk::Semaphore> vulkan_semaphores_start{vulkan_swapchain_images.size()};
	std::vector<vk::Semaphore> vulkan_semaphores_finished{vulkan_swapchain_images.size()};
	for (size_t i = 0; i < vulkan_swapchain_images.size(); ++i)
	{
		vulkan_semaphores_start[i] = vulkan_device.createSemaphore({});
		vulkan_semaphores_finished[i] = vulkan_device.createSemaphore({});
	}

	size_t vulkan_frame_idx = 0;

	while (win32_running) 
	{
		MSG win32_message;
		if (PeekMessageW(&win32_message, win32_window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&win32_message);
			DispatchMessageW(&win32_message);
			continue;
		}

		// TODO: Do something better than just break.
		if (vulkan_device.waitForFences(
			{vulkan_fences[vulkan_frame_idx]}, 
			vk::True, 
			std::numeric_limits<uint64_t>::max())
			!= vk::Result::eSuccess)
		{
			break;
		}

		vulkan_device.resetFences({vulkan_fences[vulkan_frame_idx]});

		uint32_t vulkan_image_idx = *vulkan_device.acquireNextImageKHR(vulkan_swapchain, std::numeric_limits<uint64_t>::max(), vulkan_semaphores_start[vulkan_frame_idx]);

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

		
	}

	return 0;
}