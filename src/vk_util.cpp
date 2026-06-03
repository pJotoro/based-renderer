#include "vk_util_pch.hpp"

#include "macros.hpp"
#include "util.hpp"

namespace based_renderer
{
#if BASED_RENDERER_VK_LAYERS
	std::vector<char const *> vk_get_instance_layers()
	{
		std::vector<char const *> instance_layers;

		std::vector<vk::LayerProperties> layer_properties = vk::enumerateInstanceLayerProperties();
		for (vk::LayerProperties const &layer_property : layer_properties)
		{
			// TODO: Don't check for each of these every single time. There is no reason to do that.
			if (std::strcmp(layer_property.layerName, "VK_LAYER_LUNARG_monitor") == 0)
			{
				instance_layers.push_back("VK_LAYER_LUNARG_monitor");
			}
			else if (std::strcmp(layer_property.layerName, "VK_LAYER_KHRONOS_validation") == 0)
			{
				instance_layers.push_back("VK_LAYER_KHRONOS_validation");
			}
		}

		return instance_layers;
	}
#endif

	std::vector<char const *> vk_get_instance_extensions()
	{
		std::vector<char const *> instance_extensions;

		instance_extensions.push_back("VK_KHR_surface");
		instance_extensions.push_back(VK_KHR_platform_surface);
	#if BASED_RENDERER_VK_DEBUG_OUTPUT
		instance_extensions.push_back("VK_EXT_debug_utils");
	#endif
		std::vector<vk::ExtensionProperties> instance_extension_properties = vk::enumerateInstanceExtensionProperties();
		std::vector<std::string> missing_instance_extensions;
		for (char const *instance_extension : instance_extensions)
		{
			bool found = false;
			for (vk::ExtensionProperties const &extension_properties : instance_extension_properties)
			{
				if (std::strcmp(extension_properties.extensionName, instance_extension) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				missing_instance_extensions.push_back(instance_extension);
			}
		}
		if (missing_instance_extensions.size() > 0)
		{
			throw vk::ExtensionNotPresentError{FORMAT_ERROR(to_string(missing_instance_extensions))};
		}

		return instance_extensions;
	}

	std::vector<char const *> vk_get_device_extensions(vk::PhysicalDevice const physical_device)
	{
		std::vector<char const *> device_extensions;

		device_extensions.push_back("VK_KHR_swapchain");
		auto device_extension_properties = physical_device.enumerateDeviceExtensionProperties();
		std::vector<std::string> missing_device_extensions;
		for (char const *device_extension : device_extensions)
		{
			bool found = false;
			for (vk::ExtensionProperties const &extension_properties : device_extension_properties)
			{
				if (std::strcmp(extension_properties.extensionName, device_extension) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				missing_device_extensions.push_back(device_extension);
			}
		}
		if (missing_device_extensions.size() > 0)
		{
			throw vk::ExtensionNotPresentError{FORMAT_ERROR(to_string(missing_device_extensions))};
		}

		return device_extensions;
	}

	// TODO: This is stupid. Find out how queue priorities should be done.
	static std::array<float, 64> vk_queue_priorities{
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	std::vector<vk::DeviceQueueCreateInfo> vk_get_device_queue_infos(std::vector<vk::QueueFamilyProperties> const &queue_family_properties)
	{
		std::vector<vk::DeviceQueueCreateInfo> device_queue_infos;
		device_queue_infos.reserve(queue_family_properties.size());

		for (size_t i = 0; i < queue_family_properties.size(); ++i)
		{
			if (queue_family_properties[i].queueCount > 0)
			{
				device_queue_infos.push_back(vk::DeviceQueueCreateInfo{
					{},
					static_cast<uint32_t>(i),
					queue_family_properties[i].queueCount,
					vk_queue_priorities.data(),
				});
			}
		}

		return device_queue_infos;
	}

	std::vector<std::vector<vk::Queue>> vk_get_queues(vk::Device const device, std::vector<vk::QueueFamilyProperties> const &queue_family_properties)
	{
		std::vector<std::vector<vk::Queue>> queues{queue_family_properties.size()};

		for (size_t i = 0; i < queue_family_properties.size(); ++i)
		{
			queues[i].resize(queue_family_properties[i].queueCount);
			for (size_t j = 0; j < static_cast<size_t>(queue_family_properties[i].queueCount); ++j)
			{
				queues[i][j] = device.getQueue(static_cast<uint32_t>(i), static_cast<uint32_t>(j));
			}
		}

		return queues;
	}

	size_t vk_find_queue_family_idx(std::vector<vk::QueueFamilyProperties> const &queue_family_properties, vk::QueueFlagBits const flags)
	{
		for (size_t i = 0; i < queue_family_properties.size(); ++i)
		{
			if ((queue_family_properties[i].queueFlags & vk::QueueFlags{flags}) != vk::QueueFlags{})
			{
				return i;
			}
		}
		// TODO: Is there a different error we should be using here?
		throw vk::LogicError{FORMAT_ERROR("Failed to find queue family idx")};
	}

	// struct VulkanMemoryTypeInfo
	// {
	// 	uint32_t idx;
	// 	vk::MemoryPropertyFlags properties;
	// };

	// static std::optional<VulkanMemoryTypeInfo> vk_get_memory_type_info(
	// 	vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
	// 	uint32_t const memory_type_bits,
	// 	vk::MemoryPropertyFlags desired_memory_properties) noexcept
	// {
	// 	std::optional<VulkanMemoryTypeInfo> res;

	// 	for (
	// 		uint32_t memory_type_idx = 0; 
	// 		memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
	// 		++memory_type_idx)
	// 	{
	// 		uint32_t memory_type_bit = 1 << memory_type_idx;		
	// 		vk::MemoryPropertyFlags memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
	// 		if ((memory_type_bits&memory_type_bit) && (desired_memory_properties&memory_properties))
	// 		{
	// 			res = VulkanMemoryTypeInfo{
	// 				.idx = memory_type_idx, 
	// 				.properties = memory_properties,
	// 			};
	// 			break;
	// 		}
	// 	}

	// 	return res;
	// }

	// static std::optional<VulkanMemoryTypeInfo> vk_get_memory_type_info(
	// 	vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
	// 	uint32_t const memory_type_bits,
	// 	vk::BufferUsageFlags const usage) noexcept
	// {
	// 	vk::MemoryPropertyFlags desired_memory_properties;
	// 	if (usage&vk::BufferUsageFlagBits::eTransferSrc)
	// 	{
	// 		desired_memory_properties = vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
	// 	}
	// 	else if (usage&vk::BufferUsageFlagBits::eTransferDst)
	// 	{
	// 		desired_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
	// 	}
	// 	else
	// 	{
	// 		desired_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal|vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
	// 	}

	// 	return vk_get_memory_type_info(physical_device_memory_properties, memory_type_bits, desired_memory_properties);
	// }

	// static std::optional<VulkanMemoryTypeInfo> vk_get_memory_type_info(
	// 	vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
	// 	uint32_t const memory_type_bits,
	// 	vk::ImageUsageFlags const usage) noexcept
	// {
	// 	vk::MemoryPropertyFlags desired_memory_properties;
	// 	if (usage&vk::ImageUsageFlagBits::eTransferSrc)
	// 	{
	// 		desired_memory_properties = vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
	// 	}
	// 	else if (usage&vk::ImageUsageFlagBits::eTransferDst)
	// 	{
	// 		desired_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
	// 	}
	// 	else
	// 	{
	// 		if (usage&vk::ImageUsageFlagBits::eDepthStencilAttachment)
	// 		{
	// 			desired_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
	// 		}
	// 		else
	// 		{
	// 			desired_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal|vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
	// 		}
	// 	}

	// 	return vk_get_memory_type_info(physical_device_memory_properties, memory_type_bits, desired_memory_properties);
	// }

	// struct VulkanStagingBufferAllocation
	// {
	// 	vk::DeviceMemory memory;
	// 	vk::DeviceSize offset;
	// 	vk::DeviceSize size;
	// 	vk::DeviceSize align;
	// 	VulkanMemoryTypeInfo memory_type_info;
	// 	bool dedicated_allocation;

	// 	vk::Buffer handle;
	// };

	// // You might be wondering why we are not using inheritance here, since each of these structs share so many members. The reason is simply convenience. Specifically, it's not possible to initialize staging_buffer.memory_type_info.idx using a designated initializer if memory_type_info is a member of the parent class.

	// struct VulkanBufferAllocation
	// {
	// 	vk::DeviceMemory memory;
	// 	vk::DeviceSize offset;
	// 	vk::DeviceSize size;
	// 	vk::DeviceSize align;
	// 	VulkanMemoryTypeInfo memory_type_info;
	// 	bool dedicated_allocation;

	// 	vk::Buffer handle;
	// 	VulkanStagingBufferAllocation staging_buffer{
	// 		.memory_type_info = VulkanMemoryTypeInfo{
	// 			.idx = 0xFFFFFFFF,
	// 		},
	// 	};

	// 	bool has_staging_buffer()
	// 	{
	// 		return staging_buffer.memory_type_info.idx != 0xFFFFFFFF;
	// 	}

	// 	// The idea is, you can always treat a buffer as if it is a staging buffer, even if it's not.
	// 	vk::DeviceMemory get_staging_buffer_memory()
	// 	{
	// 		if (has_staging_buffer())
	// 		{
	// 			return staging_buffer.memory;
	// 		}
	// 		else
	// 		{
	// 			return memory;
	// 		}
	// 	}
	// };

	// struct VulkanImageAllocation
	// {
	// 	vk::DeviceMemory memory;
	// 	vk::DeviceSize offset;
	// 	vk::DeviceSize size;
	// 	vk::DeviceSize align;
	// 	VulkanMemoryTypeInfo memory_type_info;
	// 	bool dedicated_allocation;

	// 	vk::Image handle;
	// 	VulkanStagingBufferAllocation staging_buffer{
	// 		.memory_type_info = VulkanMemoryTypeInfo{
	// 			.idx = 0xFFFFFFFF,
	// 		},
	// 	};

	// 	bool has_staging_buffer()
	// 	{
	// 		return staging_buffer.memory_type_info.idx != 0xFFFFFFFF;
	// 	}
	// };

	// // https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/#heading-2-5
	// static bool is_power_of_2(vk::DeviceSize const x) 
	// {
	// 	return (x & (x-1)) == 0;
	// }
	// static vk::DeviceSize align_forward(vk::DeviceSize offset, vk::DeviceSize const align) 
	// {
	// 	if (!is_power_of_2(align))
	// 	{
	// 		std::string message{std::format("{} is not a power of 2.", align)};
	// 		throw vk::LogicError{FORMAT_ERROR(message)};
	// 	}

	// 	// Same as (offset % align) but faster as 'align' is a power of two
	// 	vk::DeviceSize modulo = offset & (align-1);

	// 	if (modulo != 0) 
	// 	{
	// 		// If 'offset' is not aligned, push it to the
	// 		// next value that is aligned
	// 		offset += align - modulo;
	// 	}
	// 	return offset;
	// }

	// void vk_allocate(
	// 	/* in */ vk::Device const device,
	// 	/* in */ vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
	// 	/* in */ std::span<vk::BufferCreateInfo> buffer_create_infos,
	// 	/* in */ std::span<vk::ImageCreateInfo> image_create_infos,
	// 	/* out */ std::span<VulkanBufferAllocation/*, buffer_create_infos.size()*/> buffer_allocations,
	// 	/* out */ std::span<VulkanImageAllocation/*, image_create_infos.size()*/> image_allocations) 
	// {
	// 	// TODO: Should I do a warning here?
	// 	size_t buffer_count = std::min(buffer_create_infos.size(), buffer_allocations.size());
	// 	size_t image_count = std::min(image_create_infos.size(), image_allocations.size());

	// 	std::vector<vk::BindBufferMemoryInfo> bind_buffer_memory_infos;
	// 	bind_buffer_memory_infos.reserve(buffer_count);
	// 	std::vector<vk::BindImageMemoryInfo> bind_image_memory_infos;
	// 	bind_image_memory_infos.reserve(image_count);

	// 	// TODO (or possibly a mistake in the Vulkan C++ bindings?): getBufferMemoryRequirements and getImageMemoryRequirements do have enhanced versions that return a vk::StructureChain, but I can't seem to use them because C++ still thinks I'm calling the other one.

	// 	for (size_t i = 0; i < buffer_count; ++i) 
	// 	{
	// 		if (buffer_create_infos[i].usage&vk::BufferUsageFlagBits::eTransferDst)
	// 		{
	// 			throw vk::LogicError{FORMAT_ERROR(
	// 				"Do not use vk::BufferUsageFlagBits::eTransferDst directly. Just let vk_allocate decide."
	// 			)};
	// 		}
	// 		if (buffer_create_infos[i].usage&vk::BufferUsageFlagBits::eTransferSrc)
	// 		{
	// 			throw vk::LogicError{FORMAT_ERROR(
	// 				"Do not use vk::BufferUsageFlagBits::eTransferSrc. vk_allocate already creates a staging buffer for each buffer when necessary."
	// 			)};
	// 		}

	// 		vk::DeviceBufferMemoryRequirements buffer_memory_requirements_info{
	// 			&buffer_create_infos[i],
	// 		};
	// 		vk::MemoryDedicatedRequirements memory_dedicated_requirements;
	// 		vk::MemoryRequirements2 buffer_memory_requirements;
	// 		buffer_memory_requirements.pNext = &memory_dedicated_requirements;
	// 		device.getBufferMemoryRequirements(
	// 			&buffer_memory_requirements_info,
	// 			&buffer_memory_requirements
	// 		);

	// 		bool needs_staging_buffer = false;
	// 		std::optional<VulkanMemoryTypeInfo> maybe_memory_type_info = vk_get_memory_type_info(
	// 			physical_device_memory_properties,
	// 			buffer_memory_requirements.memoryRequirements.memoryTypeBits,
	// 			buffer_create_infos[i].usage
	// 		);
	// 		if (!maybe_memory_type_info.has_value())
	// 		{
	// 			needs_staging_buffer = true;
	// 			buffer_create_infos[i].usage |= vk::BufferUsageFlagBits::eTransferDst;
	// 			maybe_memory_type_info = vk_get_memory_type_info(
	// 				physical_device_memory_properties,
	// 				buffer_memory_requirements.memoryRequirements.memoryTypeBits,
	// 				buffer_create_infos[i].usage
	// 			);
	// 			if (!maybe_memory_type_info.has_value())
	// 			{
	// 				throw vk::LogicError{FORMAT_ERROR("Failed to find compatible memory type info.")};
	// 			}
	// 		}
	// 		VulkanMemoryTypeInfo memory_type_info = maybe_memory_type_info.value();

	// 		VulkanBufferAllocation &buffer_allocation = buffer_allocations[i];
	// 		buffer_allocation.handle = device.createBuffer(buffer_create_infos[i]);
	// 		buffer_allocation.size = buffer_memory_requirements.memoryRequirements.size;
	// 		buffer_allocation.align = buffer_memory_requirements.memoryRequirements.alignment;
	// 		if (needs_staging_buffer)
	// 		{
	// 			VulkanStagingBufferAllocation staging_buffer_allocation{};
	// 			staging_buffer_allocation.handle = device.createBuffer({
	// 				vk::BufferCreateFlags{},
	// 				buffer_allocation.size,
	// 				vk::BufferUsageFlagBits::eTransferSrc,
	// 			});

	// 			vk::BufferMemoryRequirementsInfo2 staging_buffer_memory_requirements_info;
	// 			staging_buffer_memory_requirements_info.buffer = staging_buffer_allocation.handle;
	// 			vk::MemoryRequirements2 staging_buffer_memory_requirements;
	// 			device.getBufferMemoryRequirements2(&staging_buffer_memory_requirements_info, &staging_buffer_memory_requirements);

	// 			staging_buffer_allocation.size = staging_buffer_memory_requirements.memoryRequirements.size;
	// 			staging_buffer_allocation.align = staging_buffer_memory_requirements.memoryRequirements.alignment;

	// 			// This should always succeed, because every implementation is guaranteed to have at least one memory type that is host visible and host coherent.
	// 			staging_buffer_allocation.memory_type_info = vk_get_memory_type_info(
	// 				physical_device_memory_properties,
	// 				staging_buffer_memory_requirements.memoryRequirements.memoryTypeBits,
	// 				vk::BufferUsageFlagBits::eTransferSrc).value();

	// 			buffer_allocation.staging_buffer = staging_buffer_allocation;
	// 		}

	// 		// if (memory_dedicated_requirements.prefersDedicatedAllocation || memory_dedicated_requirements.requiresDedicatedAllocation) 
	// 		// {
	// 		// 	buffer_allocation.dedicated_allocation = true;

	// 		// 	vk::MemoryDedicatedAllocateInfo memory_dedicated_allocate_info;
	// 		// 	memory_dedicated_allocate_info.buffer = buffer_allocation.handle;
				
	// 		// 	vk::MemoryAllocateInfo memory_allocate_info;
	// 		// 	memory_allocate_info.pNext = &memory_dedicated_allocate_info;
	// 		// 	memory_allocate_info.allocationSize = buffer_allocation.size;
	// 		// 	memory_allocate_info.memoryTypeIndex = buffer_allocation.memory_type_info.idx;

	// 		// 	buffer_allocation.memory = device.allocateMemory(memory_allocate_info);

	// 		// 	vk::BindBufferMemoryInfo bind_buffer_memory_info;
	// 		// 	bind_buffer_memory_info.buffer = buffer_allocation.handle;
	// 		// 	bind_buffer_memory_info.memory = buffer_allocation.memory;
	// 		// 	bind_buffer_memory_infos.push_back(bind_buffer_memory_info);
	// 		// }
	// 	}

	// 	for (size_t i = 0; i < image_count; ++i) 
	// 	{
	// 		if (image_create_infos[i].usage&vk::ImageUsageFlagBits::eTransferDst)
	// 		{
	// 			throw vk::LogicError{FORMAT_ERROR(
	// 				"Do not use vk::ImageUsageFlagBits::eTransferDst directly. Just let vk_allocate decide."
	// 			)};
	// 		}
	// 		if (image_create_infos[i].usage&vk::ImageUsageFlagBits::eTransferSrc)
	// 		{
	// 			throw vk::LogicError{FORMAT_ERROR(
	// 				"Do not use vk::ImageUsageFlagBits::eTransferSrc. vk_allocate already creates a staging buffer for each buffer when necessary."
	// 			)};
	// 		}

	// 		// TODO: Same problem as with getBufferMemoryRequirements.

	// 		vk::DeviceImageMemoryRequirements image_memory_requirements_info{
	// 			&image_create_infos[i],
	// 		};
	// 		vk::MemoryDedicatedRequirements memory_dedicated_requirements;
	// 		vk::MemoryRequirements2 image_memory_requirements;
	// 		image_memory_requirements.pNext = &memory_dedicated_requirements;
	// 		device.getImageMemoryRequirements(
	// 			&image_memory_requirements_info,
	// 			&image_memory_requirements
	// 		);

	// 		bool needs_staging_buffer = false;
	// 		std::optional<VulkanMemoryTypeInfo> maybe_memory_type_info = vk_get_memory_type_info(
	// 			physical_device_memory_properties,
	// 			image_memory_requirements.memoryRequirements.memoryTypeBits,
	// 			image_create_infos[i].usage
	// 		);
	// 		if (!maybe_memory_type_info.has_value())
	// 		{
	// 			needs_staging_buffer = true;
	// 			image_create_infos[i].usage |= vk::ImageUsageFlagBits::eTransferDst;
	// 			maybe_memory_type_info = vk_get_memory_type_info(
	// 				physical_device_memory_properties,
	// 				image_memory_requirements.memoryRequirements.memoryTypeBits,
	// 				image_create_infos[i].usage
	// 			);
	// 			if (!maybe_memory_type_info.has_value())
	// 			{
	// 				throw vk::LogicError{FORMAT_ERROR("Failed to find compatible memory type info.")};
	// 			}
	// 		}
	// 		VulkanImageAllocation &image_allocation = image_allocations[i];
	// 		image_allocation.memory_type_info = maybe_memory_type_info.value();
	// 		image_allocation.handle = device.createImage(image_create_infos[i]);
	// 		image_allocation.size = image_memory_requirements.memoryRequirements.size;
	// 		image_allocation.align = image_memory_requirements.memoryRequirements.alignment;
			
	// 		if (needs_staging_buffer)
	// 		{
	// 			VulkanStagingBufferAllocation staging_buffer_allocation{};
	// 			staging_buffer_allocation.handle = device.createBuffer({
	// 				vk::BufferCreateFlags{},
	// 				image_allocation.size,
	// 				vk::BufferUsageFlagBits::eTransferSrc,
	// 			});

	// 			vk::BufferMemoryRequirementsInfo2 staging_buffer_memory_requirements_info;
	// 			staging_buffer_memory_requirements_info.buffer = staging_buffer_allocation.handle;
				
	// 			vk::MemoryRequirements2 staging_buffer_memory_requirements;
	// 			device.getBufferMemoryRequirements2(&staging_buffer_memory_requirements_info, &staging_buffer_memory_requirements);

	// 			staging_buffer_allocation.size = staging_buffer_memory_requirements.memoryRequirements.size;
	// 			staging_buffer_allocation.align = staging_buffer_memory_requirements.memoryRequirements.alignment;

	// 			staging_buffer_allocation.memory_type_info = vk_get_memory_type_info(
	// 				physical_device_memory_properties,
	// 				staging_buffer_memory_requirements.memoryRequirements.memoryTypeBits,
	// 				vk::BufferUsageFlagBits::eTransferSrc);

	// 			image_allocation.staging_buffer = staging_buffer_allocation;
	// 		}

	// 		// if (memory_dedicated_requirements.prefersDedicatedAllocation || memory_dedicated_requirements.requiresDedicatedAllocation) 
	// 		// {
	// 		// 	image_allocation.dedicated_allocation = true;
	// 		// 	image_allocation.offset = 0;

	// 		// 	vk::MemoryDedicatedAllocateInfo memory_dedicated_allocate_info;
	// 		// 	memory_dedicated_allocate_info.image = image_allocation.handle;
				
	// 		// 	vk::MemoryAllocateInfo memory_allocate_info;
	// 		// 	memory_allocate_info.pNext = &memory_dedicated_allocate_info;
	// 		// 	memory_allocate_info.allocationSize = image_allocation.size;
	// 		// 	memory_allocate_info.memoryTypeIndex = image_allocation.memory_type_info.idx;

	// 		// 	image_allocation.memory = device.allocateMemory(memory_allocate_info);

	// 		// 	vk::BindImageMemoryInfo bind_image_memory_info;
	// 		// 	bind_image_memory_info.image = image_allocation.handle;
	// 		// 	bind_image_memory_info.memory = image_allocation.memory;
	// 		// 	bind_image_memory_infos.push_back(bind_image_memory_info);
	// 		// }
	// 	}

	// 	for (
	// 		uint32_t memory_type_idx = 0; 
	// 		memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
	// 		++memory_type_idx)
	// 	{
	// 		size_t bind_buffer_memory_infos_size = bind_buffer_memory_infos.size();
	// 		size_t bind_image_memory_infos_size = bind_image_memory_infos.size();

	// 		vk::DeviceSize memory_offset = 0;

	// 		for (size_t i = 0; i < buffer_count; ++i) 
	// 		{
	// 			VulkanBufferAllocation const &buffer_allocation = buffer_allocations[i];

	// 			if (buffer_allocation.memory_type_info.idx == memory_type_idx /*&& !buffer_allocation.memory */) 
	// 			{
	// 				memory_offset = align_forward(memory_offset, buffer_allocation.align);

	// 				vk::BindBufferMemoryInfo bind_buffer_memory_info;
	// 				bind_buffer_memory_info.buffer = buffer_allocation.handle;
	// 				bind_buffer_memory_info.memoryOffset = memory_offset;
	// 				bind_buffer_memory_infos.push_back(bind_buffer_memory_info);

	// 				memory_offset += buffer_allocation.size;
	// 			}

	// 			if (buffer_allocation.staging_buffer.memory_type_info.idx == memory_type_idx)
	// 			{
	// 				memory_offset = align_forward(memory_offset, buffer_allocation.staging_buffer.align);

	// 				vk::BindBufferMemoryInfo bind_buffer_memory_info;
	// 				bind_buffer_memory_info.buffer = buffer_allocation.staging_buffer.handle;
	// 				bind_buffer_memory_info.memoryOffset = memory_offset;
	// 				bind_buffer_memory_infos.push_back(bind_buffer_memory_info);

	// 				memory_offset += buffer_allocation.staging_buffer.size;
	// 			}
	// 		}

	// 		for (size_t i = 0; i < image_count; ++i)
	// 		{
	// 			VulkanImageAllocation const &image_allocation = image_allocations[i];

	// 			if (image_allocation.memory_type_info.idx == memory_type_idx /*&& !image_allocation.memory */) 
	// 			{
	// 				memory_offset = align_forward(memory_offset, image_allocation.align);

	// 				vk::BindImageMemoryInfo bind_image_memory_info;
	// 				bind_image_memory_info.image = image_allocation.handle;
	// 				bind_image_memory_info.memoryOffset = memory_offset;
	// 				bind_image_memory_infos.push_back(bind_image_memory_info);

	// 				memory_offset += image_allocation.size;
	// 			}

	// 			if (image_allocation.staging_buffer.memory_type_info.idx == memory_type_idx) 
	// 			{
	// 				memory_offset = align_forward(memory_offset, image_allocation.staging_buffer.align);

	// 				vk::BindBufferMemoryInfo bind_staging_buffer_memory_info;
	// 				bind_staging_buffer_memory_info.buffer = image_allocation.staging_buffer.handle;
	// 				bind_staging_buffer_memory_info.memoryOffset = memory_offset;
	// 				bind_buffer_memory_infos.push_back(bind_staging_buffer_memory_info);

	// 				memory_offset += image_allocation.staging_buffer.size;
	// 			}
	// 		}

	// 		if (memory_offset > 0)
	// 		{
	// 			vk::MemoryAllocateInfo memory_allocate_info;
	// 			memory_allocate_info.allocationSize = memory_offset;
	// 			memory_allocate_info.memoryTypeIndex = memory_type_idx;
	// 			vk::DeviceMemory memory = device.allocateMemory(memory_allocate_info);

	// 			for (size_t i = 0; i < buffer_count; ++i) 
	// 			{
	// 				VulkanBufferAllocation &buffer_allocation = buffer_allocations[i];
	// 				if (buffer_allocation.memory_type_info.idx == memory_type_idx && !buffer_allocation.memory) 
	// 				{
	// 					buffer_allocation.memory = memory;
	// 				}

	// 				if (buffer_allocation.staging_buffer.memory_type_info.idx == memory_type_idx) 
	// 				{
	// 					buffer_allocation.staging_buffer.memory = memory;
	// 				}
	// 			}
	// 			for (size_t i = 0; i < image_count; ++i)
	// 			{
	// 				VulkanImageAllocation &image_allocation = image_allocations[i];
	// 				if (image_allocation.memory_type_info.idx == memory_type_idx && !image_allocation.memory) 
	// 				{
	// 					image_allocation.memory = memory;
	// 				}

	// 				if (image_allocation.staging_buffer.memory_type_info.idx == memory_type_idx) 
	// 				{
	// 					image_allocation.staging_buffer.memory = memory;
	// 				}
	// 			}
	// 			for (size_t i = bind_buffer_memory_infos_size; i < bind_buffer_memory_infos.size(); ++i) 
	// 			{
	// 				bind_buffer_memory_infos[i].memory = memory;
	// 			}
	// 			for (size_t i = bind_image_memory_infos_size; i < bind_image_memory_infos.size(); ++i) 
	// 			{
	// 				bind_image_memory_infos[i].memory = memory;
	// 			}
	// 		}
	// 	}

	// 	device.bindBufferMemory2(bind_buffer_memory_infos);
	// 	device.bindImageMemory2(bind_image_memory_infos);
	// }
}