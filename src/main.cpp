#include "main_pch.hpp"

#include "macros.hpp"
#include "util.hpp"

namespace based_renderer
{
	// Defined in vk_util.cpp
	// We will only ever use them in this file, so there's no reason to make a separate header.
#if BASED_RENDERER_VK_LAYERS
	std::vector<char const *> vk_get_instance_layers();
#endif
	std::vector<char const *> vk_get_instance_extensions();
	std::vector<char const *> vk_get_device_extensions(vk::PhysicalDevice const physical_device);
	std::vector<vk::DeviceQueueCreateInfo> vk_get_device_queue_infos(std::vector<vk::QueueFamilyProperties> const &queue_family_properties);
	std::vector<std::vector<vk::Queue>> vk_get_queues(vk::Device const device, std::vector<vk::QueueFamilyProperties> const &queue_family_properties);
	size_t vk_find_queue_family_idx(std::vector<vk::QueueFamilyProperties> const &queue_family_properties, vk::QueueFlagBits const flags);

	static std::system_error win32_system_error() noexcept
	{
		std::error_code error_code{static_cast<int>(GetLastError()), std::system_category()};
		std::system_error system_error{error_code};
		return system_error;
	}

	// TODO: Remove globals.
	static bool win32_running;
	static bool should_rotate = true;
	static int32_t key_w;
	static int32_t key_s;
	static int32_t key_a;
	static int32_t key_d;
	static glm::ivec2 mouse_pos;

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
			case WM_KEYDOWN: 
				{
				uint8_t virtual_key_code = static_cast<uint8_t>(win32_w_param);
				switch (virtual_key_code)
				{
					case VK_SPACE:
					{
						should_rotate = !should_rotate;
					} break;
					case 'W':
					{
						key_w = true;
					} break;
					case 'S':
					{
						key_s = true;
					} break;
					case 'A':
					{
						key_a = true;
					} break;
					case 'D':
					{
						key_d = true;
					} break;
					case VK_ESCAPE: 
					{
						win32_running = false;
					} break;
				}
			} break;
			case WM_KEYUP: 
			{
				uint8_t virtual_key_code = static_cast<uint8_t>(win32_w_param);
				switch (virtual_key_code)
				{
					case 'W':
					{
						key_w = false;
					} break;
					case 'S':
					{
						key_s = false;
					} break;
					case 'A':
					{
						key_a = false;
					} break;
					case 'D':
					{
						key_d = false;
					} break;
					case VK_ESCAPE: 
					{
						win32_running = false;
					} break;
				}
			} break;
			case WM_MOUSEMOVE:
			{
				memcpy(&mouse_pos, &win32_l_param, sizeof(LPARAM));
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

	vk::Bool32 VKAPI_PTR vk_debug_callback(
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

	

	static uint32_t vk_find_memory_type_idx(
		vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
		uint32_t const memory_type_bits,
		vk::MemoryPropertyFlags const desired_memory_properties)
	{
		for (
			uint32_t memory_type_idx = 0; 
			memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
			++memory_type_idx)
		{
			uint32_t memory_type_bit = 1 << memory_type_idx;		
			vk::MemoryPropertyFlags memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
			if ((memory_type_bits&memory_type_bit) && ((desired_memory_properties&memory_properties) == desired_memory_properties))
			{
				return memory_type_idx;
			}
		}

		throw vk::LogicError{FORMAT_ERROR("Failed to find memory type index with the desired memory properties!")};
	}

	#define SLANG_CHECK(RESULT) STMT( \
		switch (RESULT) \
		{ \
			case SLANG_OK: \
				break; \
			case SLANG_FAIL: \
				throw std::runtime_error{FORMAT_ERROR("Slang: failed for unknown reason.")}; \
			case SLANG_E_NOT_IMPLEMENTED: \
				throw std::logic_error{FORMAT_ERROR("Slang: function not implemented.")}; \
			case SLANG_E_NO_INTERFACE: \
				throw std::logic_error{FORMAT_ERROR("Slang: no interface.")}; \
			case SLANG_E_ABORT: \
				throw std::runtime_error{FORMAT_ERROR("Slang: error was aborted.")}; \
			case SLANG_E_INVALID_HANDLE: \
				throw std::logic_error{FORMAT_ERROR("Slang: invalid handle")}; \
			case SLANG_E_INVALID_ARG: \
				throw std::invalid_argument{FORMAT_ERROR("Slang: invalid argument.")}; \
			case SLANG_E_OUT_OF_MEMORY: \
				throw std::runtime_error{FORMAT_ERROR("Slang: ran out of memory.")}; \
		} \
	)

	// TODO: Remove global variable.
	static HINSTANCE win32_instance;

	static void main();
}

int WINAPI WinMain(
	HINSTANCE win32_instance,
	HINSTANCE win32_prev_instance,
	LPSTR win32_command_line,
	int	win32_show_command)
{
	UNUSED(win32_prev_instance);
	UNUSED(win32_command_line);
	UNUSED(win32_show_command);

	based_renderer::win32_instance = win32_instance;

	try
	{
		based_renderer::main();
	}
	catch (vk::OutOfHostMemoryError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::OutOfHostMemoryError");
	}
	catch (vk::OutOfDeviceMemoryError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::OutOfDeviceMemoryError");
	}
	catch (vk::InitializationFailedError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::InitializationFailedError");
	}
	catch (vk::DeviceLostError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::DeviceLostError");
	}
	catch (vk::MemoryMapFailedError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::MemoryMapFailedError");
	}
	catch (vk::LayerNotPresentError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::LayerNotPresentError");
	}
	catch (vk::ExtensionNotPresentError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::ExtensionNotPresentError");
	}
	catch (vk::FeatureNotPresentError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::FeatureNotPresentError");
	}
	catch (vk::IncompatibleDriverError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::IncompatibleDriverError");
	}
	catch (vk::TooManyObjectsError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::TooManyObjectsError");
	}
	catch (vk::FormatNotSupportedError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::FormatNotSupportedError");
	}
	catch (vk::FragmentedPoolError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::FragmentedPoolError");
	}
	catch (vk::UnknownError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::UnknownError");
	}
	catch (vk::ValidationFailedError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::ValidationFailedError");
	}
	catch (vk::OutOfPoolMemoryError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::OutOfPoolMemoryError");
	}
	catch (vk::InvalidExternalHandleError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::InvalidExternalHandleError");
	}
	catch (vk::InvalidOpaqueCaptureAddressError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::InvalidOpaqueCaptureAddressError");
	}
	catch (vk::FragmentationError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::FragmentationError");
	}
	catch (vk::NotPermittedError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::NotPermittedError");
	}
	catch (vk::SurfaceLostKHRError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::SurfaceLostKHRError");
	}
	catch (vk::NativeWindowInUseKHRError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::NativeWindowInUseKHRError");
	}
	catch (vk::OutOfDateKHRError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::OutOfDateKHRError");
	}
	catch (vk::InvalidShaderNVError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::InvalidShaderNVError");
	}
	catch (vk::FullScreenExclusiveModeLostEXTError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::FullScreenExclusiveModeLostEXTError");

		// NOTE: We are not actually using this
		// extension yet, but will be soon.
	}
	catch (vk::LogicError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::LogicError");
	}
	catch (vk::SystemError err)
	{
		based_renderer::win32_message_box(err.what(), "vk::SystemError");
	}

	catch (std::invalid_argument err)
	{
		based_renderer::win32_message_box(err.what(), "std::invalid_argument");
	}
	catch (std::domain_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::domain_error");
	}
	catch (std::length_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::length_error");
	}
	catch (std::out_of_range err)
	{
		based_renderer::win32_message_box(err.what(), "std::out_of_range");
	}
	catch (std::range_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::range_error");
	}
	catch (std::overflow_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::overflow_error");
	}
	catch (std::underflow_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::underflow_error");
	}
	catch (std::logic_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::logic_error");
	}
	catch (std::runtime_error err)
	{
		based_renderer::win32_message_box(err.what(), "std::runtime_error");
	}
	catch (...)
	{
		based_renderer::win32_message_box("Failed for unknown reason.", "Error");
	}

	return 0;
}

namespace based_renderer 
{
	static void vk_map_memory(vk::Device const device, vk::DeviceMemory const device_memory, void const *memory, vk::DeviceSize const memory_size)
	{
	    void *data;
		vk::detail::resultCheck(
			device.mapMemory(
				device_memory,
				0, 
				memory_size,
				vk::MemoryMapFlags{}, 
				&data
			), 
			"Failed to map memory!"
		);
		memcpy(data, memory, memory_size);
		device.unmapMemory(device_memory);
	}

	template <typename T>
	static void vk_map_memory(vk::Device const device, vk::DeviceMemory const device_memory, std::vector<T> const &memory)
	{
		vk_map_memory(device, device_memory, memory.data(), sizeof(T)*memory.size());
	}

	struct Uniforms
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	float constexpr TAU = 6.28318530717958647693f;

	// See projection.docx
	static glm::mat4 perspective(float const aspect_ratio) noexcept
	{
		glm::mat4 res{1.0f};
		res[0][0] = 1.73205080756887729353f/aspect_ratio;
		res[1][1] = 1.73205080756887729353f;
		res[2][2] = 2.0e-20f;
		res[3][2] = 0.09999999999999999998f;
		res[2][3] = 1.0f;
		return res;
	}

	#if 0
	static void update_cube(
		vk::Device const device, 
		vk::DeviceMemory const uniforms_memory, 
		Uniforms &uniforms,
		float const dt) noexcept
	{
		if (should_rotate)
		{
			uniforms.model = glm::rotate(uniforms.model, dt, glm::normalize(glm::vec3{3.0f, 2.0f, 1.0f}));
		}

		// Translate based on whether WASD keys are pressed.
		int32_t const cube_dir_z = key_s - key_w;
		int32_t const cube_dir_x = key_a - key_d;
		uniforms.view = glm::translate(uniforms.view, glm::vec3{static_cast<float>(cube_dir_x)*dt, 0.0f, static_cast<float>(cube_dir_z)*dt});

		// Rotate based on mouse delta (doesn't work at all right now).
		static glm::ivec2 last_mouse_pos{-1, -1};
		if (last_mouse_pos == glm::ivec2{-1, -1})
		{
			last_mouse_pos = mouse_pos;
		}
		glm::vec2 mouse_pos_diff = glm::vec2{mouse_pos - last_mouse_pos};
		uniforms.view = glm::rotate(uniforms.view, mouse_pos_diff.x*dt/(TAU*2048.0f), glm::vec3{1.0f, 0.0f, 0.0f});
		uniforms.view = glm::rotate(uniforms.view, mouse_pos_diff.y*dt/(TAU*2048.0f), glm::vec3{0.0f, 0.0f, 1.0f});
		last_mouse_pos = mouse_pos;

		vk_map_memory(device, uniforms_memory, &uniforms, sizeof(uniforms));
	}
	#endif

	static cgltf_data *gltf_load(char const *path)
	{
		cgltf_data *data;
		cgltf_options options{};
		cgltf_result res = cgltf_parse_file(&options, path, &data);
		if (res != cgltf_result_success)
		{
			// TODO: Change this to make it tell you which file failed to load.
			throw std::runtime_error{FORMAT_ERROR("Failed to load gltf file")};
		}

		return data;
	}

	#if 0
	static glm::mat4 gltf_node_transform_local(cgltf_node const *node)
	{
		glm::mat4 res;
		cgltf_node_transform_local(node, &res);
		return res;
	}
	#endif

	static glm::mat4 gltf_node_transform_world(cgltf_node const *node)
	{
		glm::mat4 res;
		cgltf_node_transform_world(node, reinterpret_cast<float *>(&res));
		return res;
	}

	static void vk_modify_physical_device_features(auto &physical_device_features)
	{
		std::vector<std::string> missing_features;
		#define BASED_RENDERER_VK_REQUIRE_FEATURE(FEATURE) STMT( \
			if (!features.FEATURE) \
			{ \
				missing_features.push_back(STRINGIFY(FEATURE)); \
			} \
		)
		#define BASED_RENDERER_VK_DISABLE_FEATURE(FEATURE) STMT( \
			features.FEATURE = vk::False; \
		)
		// You might say: this doesn't do anything. Why define it?
		// It's so that way, when scrolling through all of the Vulkan
		// features I might want to use, none of them are missing.
		#define BASED_RENDERER_VK_ALLOW_FEATURE(FEATURE)

		{
			auto &features = std::get<0>(physical_device_features).features;
			BASED_RENDERER_VK_DISABLE_FEATURE(robustBufferAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(fullDrawIndexUint32);
			BASED_RENDERER_VK_DISABLE_FEATURE(imageCubeArray);
			BASED_RENDERER_VK_DISABLE_FEATURE(independentBlend);
			BASED_RENDERER_VK_DISABLE_FEATURE(geometryShader);
			BASED_RENDERER_VK_DISABLE_FEATURE(tessellationShader);
			BASED_RENDERER_VK_DISABLE_FEATURE(sampleRateShading);
			BASED_RENDERER_VK_DISABLE_FEATURE(dualSrcBlend);
			BASED_RENDERER_VK_DISABLE_FEATURE(logicOp);
			BASED_RENDERER_VK_DISABLE_FEATURE(multiDrawIndirect);
			BASED_RENDERER_VK_DISABLE_FEATURE(drawIndirectFirstInstance);
			BASED_RENDERER_VK_DISABLE_FEATURE(depthClamp);
			BASED_RENDERER_VK_DISABLE_FEATURE(depthBiasClamp);
			BASED_RENDERER_VK_DISABLE_FEATURE(fillModeNonSolid);
			BASED_RENDERER_VK_DISABLE_FEATURE(depthBounds);
			BASED_RENDERER_VK_DISABLE_FEATURE(wideLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(largePoints);
			BASED_RENDERER_VK_DISABLE_FEATURE(alphaToOne);
			BASED_RENDERER_VK_DISABLE_FEATURE(multiViewport);
			BASED_RENDERER_VK_DISABLE_FEATURE(samplerAnisotropy);
			BASED_RENDERER_VK_DISABLE_FEATURE(textureCompressionETC2);
			BASED_RENDERER_VK_DISABLE_FEATURE(textureCompressionASTC_LDR);
			BASED_RENDERER_VK_DISABLE_FEATURE(textureCompressionBC);
			BASED_RENDERER_VK_DISABLE_FEATURE(occlusionQueryPrecise);
			BASED_RENDERER_VK_DISABLE_FEATURE(pipelineStatisticsQuery);
			BASED_RENDERER_VK_DISABLE_FEATURE(vertexPipelineStoresAndAtomics);
			BASED_RENDERER_VK_DISABLE_FEATURE(fragmentStoresAndAtomics);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderTessellationAndGeometryPointSize);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderImageGatherExtended);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageImageExtendedFormats);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageImageMultisample);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageImageReadWithoutFormat);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageImageWriteWithoutFormat);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderUniformBufferArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderSampledImageArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageBufferArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageImageArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderClipDistance);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderCullDistance);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderFloat64);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderInt64);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderInt16);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderResourceResidency);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderResourceMinLod);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseBinding);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidencyBuffer);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidencyImage2D);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidencyImage3D);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidency2Samples);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidency4Samples);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidency8Samples);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidency16Samples);
			BASED_RENDERER_VK_DISABLE_FEATURE(sparseResidencyAliased);
			BASED_RENDERER_VK_DISABLE_FEATURE(variableMultisampleRate);
			BASED_RENDERER_VK_DISABLE_FEATURE(inheritedQueries);
		}
		{
			auto &features = std::get<1>(physical_device_features);
			BASED_RENDERER_VK_DISABLE_FEATURE(storageBuffer16BitAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(uniformAndStorageBuffer16BitAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(storagePushConstant16);
			BASED_RENDERER_VK_DISABLE_FEATURE(storageInputOutput16);
			BASED_RENDERER_VK_DISABLE_FEATURE(multiview);
			BASED_RENDERER_VK_DISABLE_FEATURE(multiviewGeometryShader);
			BASED_RENDERER_VK_DISABLE_FEATURE(multiviewTessellationShader);
			BASED_RENDERER_VK_DISABLE_FEATURE(variablePointersStorageBuffer);
			BASED_RENDERER_VK_DISABLE_FEATURE(variablePointers);
			BASED_RENDERER_VK_DISABLE_FEATURE(protectedMemory);
			BASED_RENDERER_VK_DISABLE_FEATURE(samplerYcbcrConversion);
			BASED_RENDERER_VK_REQUIRE_FEATURE(shaderDrawParameters); // Slang requires this.
		}
		{
			auto &features = std::get<2>(physical_device_features);
			BASED_RENDERER_VK_DISABLE_FEATURE(samplerMirrorClampToEdge);
			BASED_RENDERER_VK_DISABLE_FEATURE(drawIndirectCount);
			BASED_RENDERER_VK_DISABLE_FEATURE(storageBuffer8BitAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(uniformAndStorageBuffer8BitAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(storagePushConstant8);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderBufferInt64Atomics);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderSharedInt64Atomics);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderFloat16);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderInt8);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderInputAttachmentArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderUniformTexelBufferArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageTexelBufferArrayDynamicIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderUniformBufferArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderSampledImageArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageBufferArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageImageArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderInputAttachmentArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderUniformTexelBufferArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderStorageTexelBufferArrayNonUniformIndexing);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingUniformBufferUpdateAfterBind); // TODO: If I remember correctly, my laptop does not support this feature, but it supports basically all the other "UpdateAfterBind" features. Does this matter? Can I still put the uniform buffer in the same descriptor set as every other descriptor, as long as I don't update it after binding?
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingStorageImageUpdateAfterBind);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingStorageBufferUpdateAfterBind);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingUniformTexelBufferUpdateAfterBind);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingStorageTexelBufferUpdateAfterBind);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingUpdateUnusedWhilePending);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingPartiallyBound);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingVariableDescriptorCount);
			BASED_RENDERER_VK_DISABLE_FEATURE(runtimeDescriptorArray);
			BASED_RENDERER_VK_DISABLE_FEATURE(samplerFilterMinmax);
			BASED_RENDERER_VK_DISABLE_FEATURE(scalarBlockLayout);
			BASED_RENDERER_VK_DISABLE_FEATURE(imagelessFramebuffer);
			BASED_RENDERER_VK_DISABLE_FEATURE(uniformBufferStandardLayout);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderSubgroupExtendedTypes);
			BASED_RENDERER_VK_DISABLE_FEATURE(separateDepthStencilLayouts);
			BASED_RENDERER_VK_DISABLE_FEATURE(hostQueryReset);
			BASED_RENDERER_VK_DISABLE_FEATURE(timelineSemaphore);
			BASED_RENDERER_VK_DISABLE_FEATURE(bufferDeviceAddress);
			BASED_RENDERER_VK_DISABLE_FEATURE(bufferDeviceAddressCaptureReplay);
			BASED_RENDERER_VK_DISABLE_FEATURE(bufferDeviceAddressMultiDevice);
			BASED_RENDERER_VK_REQUIRE_FEATURE(vulkanMemoryModel); // TODO: Do we necessarily need these?
			BASED_RENDERER_VK_REQUIRE_FEATURE(vulkanMemoryModelDeviceScope);
			BASED_RENDERER_VK_REQUIRE_FEATURE(vulkanMemoryModelAvailabilityVisibilityChains);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderOutputViewportIndex);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderOutputLayer);
			BASED_RENDERER_VK_DISABLE_FEATURE(subgroupBroadcastDynamicId);
		}
		{
			auto &features = std::get<3>(physical_device_features);
			BASED_RENDERER_VK_DISABLE_FEATURE(robustImageAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(inlineUniformBlock);
			BASED_RENDERER_VK_DISABLE_FEATURE(descriptorBindingInlineUniformBlockUpdateAfterBind);
			BASED_RENDERER_VK_ALLOW_FEATURE(pipelineCreationCacheControl);
			BASED_RENDERER_VK_DISABLE_FEATURE(privateData);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderDemoteToHelperInvocation);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderTerminateInvocation);
			BASED_RENDERER_VK_DISABLE_FEATURE(subgroupSizeControl);
			BASED_RENDERER_VK_DISABLE_FEATURE(computeFullSubgroups);
			BASED_RENDERER_VK_REQUIRE_FEATURE(synchronization2);
			BASED_RENDERER_VK_DISABLE_FEATURE(textureCompressionASTC_HDR);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderZeroInitializeWorkgroupMemory);
			BASED_RENDERER_VK_REQUIRE_FEATURE(dynamicRendering);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderIntegerDotProduct);
			BASED_RENDERER_VK_DISABLE_FEATURE(maintenance4);
		}
		{
			auto &features = std::get<4>(physical_device_features);
			BASED_RENDERER_VK_DISABLE_FEATURE(globalPriorityQuery);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderSubgroupRotate);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderSubgroupRotateClustered);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderFloatControls2);
			BASED_RENDERER_VK_DISABLE_FEATURE(shaderExpectAssume);
			BASED_RENDERER_VK_DISABLE_FEATURE(rectangularLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(bresenhamLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(smoothLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(stippledRectangularLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(stippledBresenhamLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(stippledSmoothLines);
			BASED_RENDERER_VK_DISABLE_FEATURE(vertexAttributeInstanceRateDivisor);
			BASED_RENDERER_VK_DISABLE_FEATURE(vertexAttributeInstanceRateZeroDivisor);
			BASED_RENDERER_VK_DISABLE_FEATURE(indexTypeUint8);
			BASED_RENDERER_VK_DISABLE_FEATURE(dynamicRenderingLocalRead);
			BASED_RENDERER_VK_DISABLE_FEATURE(maintenance5);
			BASED_RENDERER_VK_DISABLE_FEATURE(maintenance6);
			BASED_RENDERER_VK_DISABLE_FEATURE(pipelineProtectedAccess);
			BASED_RENDERER_VK_DISABLE_FEATURE(pipelineRobustness);
			BASED_RENDERER_VK_DISABLE_FEATURE(hostImageCopy);
			BASED_RENDERER_VK_DISABLE_FEATURE(pushDescriptor);
		}

		if (missing_features.size() > 0)
		{
			throw vk::FeatureNotPresentError{FORMAT_ERROR(to_string(missing_features))};
		}

		#undef BASED_RENDERER_VK_REQUIRE_FEATURE
		#undef BASED_RENDERER_VK_DISABLE_FEATURE
		#undef BASED_RENDERER_VK_ALLOW_FEATURE
	}

	static void main()
	{
	#if BASED_RENDERER_VK_LAYERS
		std::vector<char const *> vk_instance_layers = vk_get_instance_layers();
	#endif

		std::vector<char const *> vk_instance_extensions = vk_get_instance_extensions();

		vk::ApplicationInfo vk_app_info{
			"based_renderer",
			VK_API_VERSION_1_0,
			"based_renderer",
			VK_API_VERSION_1_0,
			VK_API_VERSION_1_4,
		};

	#if BASED_RENDERER_VK_DEBUG_OUTPUT
		vk::DebugUtilsMessengerCreateInfoEXT vk_debug_output_info{
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
			vk_debug_callback
		};
	#endif

		vk::InstanceCreateInfo vk_instance_create_info{
			{},
			&vk_app_info,
	#if BASED_RENDERER_VK_LAYERS
			static_cast<uint32_t>(vk_instance_layers.size()),
			vk_instance_layers.data(),
	#else
			0,
			nullptr,
	#endif
			static_cast<uint32_t>(vk_instance_extensions.size()),
			vk_instance_extensions.data(),
		};

	#if BASED_RENDERER_VK_DEBUG_OUTPUT
		vk_instance_create_info.pNext = &vk_debug_output_info;
	#endif

		vk::Instance vk_instance = vk::createInstance(vk_instance_create_info);

		// Choose the first discrete GPU.
		// If there is no discrete GPU, default to the last GPU.
		vk::PhysicalDevice vk_physical_device;
		{
			std::vector<vk::PhysicalDevice> physical_devices = vk_instance.enumeratePhysicalDevices();
			vk_physical_device = *std::find_if(physical_devices.begin(), physical_devices.end(),
				[](vk::PhysicalDevice p) 
				{
					vk::PhysicalDeviceProperties props = p.getProperties();
					return props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
				}
			);
		}

		auto vk_physical_device_properties = vk_physical_device.getProperties2<
			vk::PhysicalDeviceProperties2,
			vk::PhysicalDeviceVulkan11Properties,
			vk::PhysicalDeviceVulkan12Properties,
			vk::PhysicalDeviceVulkan13Properties,
			vk::PhysicalDeviceVulkan14Properties>();

		vk::PhysicalDeviceMemoryProperties const vk_physical_device_memory_properties = vk_physical_device.getMemoryProperties();

		auto vk_physical_device_features = vk_physical_device.getFeatures2<
			vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan12Features,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceVulkan14Features>();
		vk_modify_physical_device_features(vk_physical_device_features);

		std::vector<char const *> vk_device_extensions = vk_get_device_extensions(vk_physical_device);

		std::vector<vk::QueueFamilyProperties> vk_queue_family_properties = vk_physical_device.getQueueFamilyProperties();
		std::vector<vk::DeviceQueueCreateInfo> vk_device_queue_infos = vk_get_device_queue_infos(vk_queue_family_properties);

		vk::Device vk_device = vk_physical_device.createDevice(vk::DeviceCreateInfo{
			{}, 
			vk_device_queue_infos,
			{},
			vk_device_extensions,
			{},
			&std::get<0>(vk_physical_device_features),
		});

		// Each queue family gets its own std::vector, whether or not it has any queues.
		std::vector<std::vector<vk::Queue>> vk_queues = vk_get_queues(vk_device, vk_queue_family_properties);

		size_t const vk_graphics_queue_family_idx = vk_find_queue_family_idx(vk_queue_family_properties, vk::QueueFlagBits::eGraphics);
		vk::Queue const vk_graphics_queue = vk_queues[vk_graphics_queue_family_idx][0];
		
		size_t const vk_transfer_queue_family_idx = vk_find_queue_family_idx(vk_queue_family_properties, vk::QueueFlagBits::eTransfer);
		vk::Queue const vk_transfer_queue = vk_queues[vk_transfer_queue_family_idx][0];

		vk::CommandPool vk_graphics_command_pool = vk_device.createCommandPool({
			vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
			static_cast<uint32_t>(vk_graphics_queue_family_idx),
		});

		vk::CommandPool vk_transfer_command_pool;
		if (vk_graphics_queue_family_idx != vk_transfer_queue_family_idx)
		{
			vk_transfer_command_pool = vk_device.createCommandPool({
				vk::CommandPoolCreateFlags(),
				static_cast<uint32_t>(vk_transfer_queue_family_idx),
			});
		}
		else
		{
			vk_transfer_command_pool = vk_graphics_command_pool;
		}

		HMONITOR win32_monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO monitor_info {sizeof(MONITORINFO)};
		if (!GetMonitorInfoW(win32_monitor, &monitor_info)) 
		{
			throw win32_system_error();
		}
		int32_t const monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
		int32_t const monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

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

		RECT win32_client_rect;
		DWORD win32_window_styles;
		DWORD win32_window_styles_ex;
	#if !BASED_RENDERER_FULLSCREEN
		win32_client_rect = RECT{
			.left = monitor_width/4,
			.top = monitor_height/4,
			.right = monitor_width*3/4,
			.bottom = monitor_height*3/4,
		};
		win32_window_styles = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
		win32_window_styles_ex = 0;
	#else
		win32_window_styles = WS_POPUP;
		win32_client_rect = RECT{
			.left = 0,
			.top = 0,
			.right = monitor_width,
			.bottom = monitor_height,
		};
		win32_window_styles_ex = 0;//WS_EX_TOPMOST;
	#endif
		RECT win32_window_rect = win32_client_rect;
		if (!AdjustWindowRectEx(&win32_window_rect, win32_window_styles, false, win32_window_styles_ex))
		{
			throw win32_system_error();
		}

		HWND win32_window = CreateWindowExW(
			win32_window_styles_ex,
			L"based_renderer",
			L"based_renderer",
			win32_window_styles,
			win32_window_rect.left,
			win32_window_rect.top,
			win32_window_rect.right - win32_window_rect.left,
			win32_window_rect.bottom - win32_window_rect.top,
			nullptr,
			nullptr,
			win32_instance,
			nullptr
		);
		if (!win32_window)
		{
			throw win32_system_error();
		}

		uint32_t const client_width = static_cast<uint32_t>(win32_client_rect.right - win32_client_rect.left);
		uint32_t const client_height = static_cast<uint32_t>(win32_client_rect.bottom - win32_client_rect.top);
		float const aspect_ratio = static_cast<float>(client_width)/static_cast<float>(client_height);

	    DEVMODEW win32_dev_mode = DEVMODEW{sizeof(DEVMODEW)};
	    if (!EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &win32_dev_mode))
	    {
	    	throw win32_system_error();
	    }
	    float const fixed_dt = 1.0f/static_cast<float>(win32_dev_mode.dmDisplayFrequency);

		vk::SurfaceKHR vk_surface = vk_instance.createWin32SurfaceKHR({
			{},
			win32_instance,
			win32_window,
		});

		std::optional<size_t> vk_present_queue_family_idx;
		for (size_t i = 0; i < vk_queue_family_properties.size(); ++i)
		{
			if (vk_physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), vk_surface))
			{
				vk_present_queue_family_idx = i;
				break;
			}
		}
		vk::Queue vk_present_queue = vk_queues[vk_present_queue_family_idx.value()][0];

		auto vk_surface_formats = vk_physical_device.getSurfaceFormatsKHR(vk_surface);
		vk::Format vk_swapchain_format = vk_surface_formats.front().format; // TODO

		auto vk_surface_capabilities = vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface);

		vk::Extent2D vk_swapchain_extent;
		vk_swapchain_extent.width  = std::clamp(
			client_width, 
			vk_surface_capabilities.minImageExtent.width, 
			vk_surface_capabilities.maxImageExtent.width
		);
		vk_swapchain_extent.height = std::clamp(
			client_height, 
			vk_surface_capabilities.minImageExtent.height, 
			vk_surface_capabilities.maxImageExtent.height
		);

		vk::PresentModeKHR vk_swapchain_present_mode = vk::PresentModeKHR::eFifo; // TODO

		vk::SurfaceTransformFlagBitsKHR vk_pre_transform = 
			(vk_surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? 
			vk::SurfaceTransformFlagBitsKHR::eIdentity : 
			vk_surface_capabilities.currentTransform;

		vk::CompositeAlphaFlagBitsKHR vk_composite_alpha = 
			(vk_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? 
				vk::CompositeAlphaFlagBitsKHR::ePreMultiplied : 
				(vk_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? 
					vk::CompositeAlphaFlagBitsKHR::ePostMultiplied : 
					(vk_surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? 
						vk::CompositeAlphaFlagBitsKHR::eInherit : 
						vk::CompositeAlphaFlagBitsKHR::eOpaque;

		vk::SwapchainCreateInfoKHR vk_swapchain_create_info{
			vk::SwapchainCreateFlagsKHR(),
			vk_surface,
			// TODO: Right now, you are still basically assuming that the image count will be 2.
			// IIRC, having an image count higher than two actually complicates synchronization somewhat.
			// I might be wrong though. In any case, it's worth looking into.
			std::clamp(2u, vk_surface_capabilities.minImageCount, vk_surface_capabilities.maxImageCount),
			vk_swapchain_format,
			vk::ColorSpaceKHR::eSrgbNonlinear,
			vk_swapchain_extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			{},
			vk_pre_transform,
			vk_composite_alpha,
			vk_swapchain_present_mode,
			true,
			nullptr,
		};

		vk::SwapchainKHR vk_swapchain = vk_device.createSwapchainKHR(vk_swapchain_create_info);

		std::vector<vk::Image> vk_swapchain_images = vk_device.getSwapchainImagesKHR(vk_swapchain);

		std::vector<vk::ImageView> vk_swapchain_image_views;
		vk_swapchain_image_views.reserve(vk_swapchain_images.size());
		vk::ImageViewCreateInfo vk_image_view_create_info{
			{}, 
			{},
			vk::ImageViewType::e2D, 
			vk_swapchain_format, 
			{}, 
			{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
		};
		for (vk::Image image : vk_swapchain_images)
		{
			vk_image_view_create_info.image = image;
			vk_swapchain_image_views.push_back(vk_device.createImageView(vk_image_view_create_info));
		}

		std::vector<vk::CommandBuffer> vk_graphics_command_buffers = vk_device.allocateCommandBuffers({
			vk_graphics_command_pool, 
			vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(vk_swapchain_images.size()), // NOTE: This is assuming that the image count will be 2.
		});

		vk::CommandBuffer vk_transfer_command_buffer;
		if (vk_graphics_command_pool != vk_transfer_command_pool)
		{
			std::vector<vk::CommandBuffer> v = vk_device.allocateCommandBuffers({
				vk_transfer_command_pool, 
				vk::CommandBufferLevel::ePrimary, 
				1,
			});
			vk_transfer_command_buffer = v[0];
		}
		else
		{
			// Just don't use the transfer command buffer then!
		}

		std::vector<vk::Fence> vk_fences{vk_swapchain_images.size()};
		for (vk::Fence &fence : vk_fences) 
		{
			fence = vk_device.createFence({{vk::FenceCreateFlagBits::eSignaled}});
		}

		std::vector<vk::Semaphore> vk_semaphores_wait{vk_swapchain_images.size()};
		std::vector<vk::Semaphore> vk_semaphores_signal{vk_swapchain_images.size()};
		for (size_t i = 0; i < vk_swapchain_images.size(); ++i)
		{
			vk_semaphores_wait[i] = vk_device.createSemaphore({});
			vk_semaphores_signal[i] = vk_device.createSemaphore({});
		}

		// NOTE: The TODO below was written back when we were still hardcoding the cube.
		// TODO: Is there something wrong with the way I have set up the depth buffer? When I enable backface culling, the cube doesn't render correctly at all. Or could it simply be the way that the cube faces are listed in the shader? If so, then there isn't actually anything wrong with the way the depth buffer is set up.

		vk::Format vk_depth_format = vk::Format::eD32Sfloat; // NOTE: Support for this always exists. As long as I'm not using a stencil buffer, this will work perfectly fine.
		vk::Image vk_depth_image = vk_device.createImage({
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			vk_depth_format, 
			vk::Extent3D{client_width, client_height, 1},
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
		});
		vk::MemoryRequirements vk_depth_image_memory_requirements = vk_device.getImageMemoryRequirements(vk_depth_image);
		vk::DeviceMemory vk_depth_image_memory = vk_device.allocateMemory({
			vk_depth_image_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_depth_image_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			),
		});
		vk_device.bindImageMemory(vk_depth_image, vk_depth_image_memory, 0);
		vk::ImageView vk_depth_image_view = vk_device.createImageView({
			vk::ImageViewCreateFlags{},
			vk_depth_image,
			vk::ImageViewType::e2D,
			vk_depth_format,
			vk::ComponentMapping{},
			vk::ImageSubresourceRange{
				vk::ImageAspectFlagBits::eDepth,
				0,
				1,
				0,
				1
			},
		});

	#if 0
		int32_t stone_image_width;
		int32_t stone_image_height;
		int32_t stone_image_channels;
		int32_t stone_image_desired_channels = 4;
		uint8_t *stone_image_file_data = stbi_load(
			"assets/stone.jpg", 
			&stone_image_width, 
			&stone_image_height, 
			&stone_image_channels, 
			stone_image_desired_channels);
		if (!stone_image_file_data)
		{
			throw std::runtime_error{FORMAT_ERROR("Failed to load assets/stone.jpg")};
		}

		vk::Buffer vk_staging_buffer = vk_device.createBuffer({
			vk::BufferCreateFlags{},
			static_cast<vk::DeviceSize>(stone_image_width*stone_image_height*stone_image_desired_channels),
			vk::BufferUsageFlagBits::eTransferSrc,
		});
		vk::MemoryRequirements vk_staging_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_staging_buffer);
		vk::DeviceMemory vk_staging_buffer_memory = vk_device.allocateMemory({
			vk_staging_buffer_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_staging_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent
			),
		});
		vk_device.bindBufferMemory(vk_staging_buffer, vk_staging_buffer_memory, 0);
		{
			void *data;
			vk::detail::resultCheck(
				vk_device.mapMemory(
					vk_staging_buffer_memory,
					0, 
					static_cast<vk::DeviceSize>(stone_image_width*stone_image_height*stone_image_desired_channels),
					vk::MemoryMapFlags{}, 
					&data
				), 
				"Failed to map memory!"
			);
			memcpy(data, stone_image_file_data, static_cast<size_t>(stone_image_width*stone_image_height*stone_image_desired_channels));
			vk_device.unmapMemory(vk_staging_buffer_memory);
		}

		vk::Image vk_image = vk_device.createImage({
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Unorm, 
			vk::Extent3D{static_cast<uint32_t>(stone_image_width), static_cast<uint32_t>(stone_image_height), 1},
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst|
			vk::ImageUsageFlagBits::eSampled,
		});
		vk::MemoryRequirements vk_image_memory_requirements = vk_device.getImageMemoryRequirements(vk_image);
		vk::DeviceMemory vk_image_memory = vk_device.allocateMemory({
			vk_image_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_image_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			),
		});
		vk_device.bindImageMemory(vk_image, vk_image_memory, 0);
		vk::ImageView vk_image_view = vk_device.createImageView({
			vk::ImageViewCreateFlags{},
			vk_image,
			vk::ImageViewType::e2D,
			vk::Format::eR8G8B8A8Unorm,
			vk::ComponentMapping{},
			vk::ImageSubresourceRange{
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1
			},
		});
	#endif

		//The box's binary is formatted like this: first all the vertices, then all the normals, then all the indices.
		cgltf_data const *box = gltf_load("assets/Box.glb");
		cgltf_mesh const &box_mesh = box->meshes[0];
		dprint("{}", box_mesh.name);
		cgltf_material const &box_material = box->materials[0]; // It seems like I can safely ignore the material. The only thing set is the alpha cutoff.
		dprint("{}", box_material.name);
		size_t box_vertex_count = 0;
		for (size_t i = 0; i < box->accessors_count; ++i)
		{
			cgltf_accessor const &accessor = box->accessors[i];
			if (accessor.name) dprint("{}", accessor.name);
			if (accessor.buffer_view->type != cgltf_buffer_view_type_indices)
			{
				box_vertex_count = std::max(box_vertex_count, accessor.count);
			}
		}
		vk::DeviceSize box_vertex_buffer_size = 0;
		vk::DeviceSize box_index_buffer_size = 0;
		for (size_t i = 0; i < box->buffer_views_count; ++i)
		{
			cgltf_buffer_view const &buffer_view = box->buffer_views[i];
			if (buffer_view.name) dprint("{}", buffer_view.name);
			switch (buffer_view.type)
			{
				case cgltf_buffer_view_type_vertices:
					box_vertex_buffer_size += buffer_view.size;
					break;
				case cgltf_buffer_view_type_indices:
					box_index_buffer_size += buffer_view.size;
					break;
			}
		}
		cgltf_buffer const &box_buffer = box->buffers[0];
		if (box_buffer.name) dprint("{}", box_buffer.name);
		for (size_t i = 0; i < box->nodes_count; ++i)
		{
			cgltf_node const &node = box->nodes[i];
			if (node.name) dprint("{}", node.name);
		}
		glm::mat4 box_model = gltf_node_transform_world(&box->nodes[1]); // THIS LINE OF CODE IS WHY NOTHING IS WORKING!!!
		cgltf_scene const *box_scene = box->scene;
		if (box_scene->name) dprint("{}", box_scene->name);

		struct Vertex
		{
			glm::vec4 pos;
			glm::vec4 normal;
		};
		std::vector<Vertex> vertices;
		vertices.reserve(box_vertex_count);
		for (size_t i = 0; i < box_vertex_count; ++i)
		{
			Vertex vertex;
			// TODO: How would this be made generic?
			auto normal = reinterpret_cast<glm::vec3 const *const>(reinterpret_cast<uint8_t const *const>(box->bin) + box->accessors[1].offset + i*box->accessors[1].stride);
			auto pos = reinterpret_cast<glm::vec3 const *const>(reinterpret_cast<uint8_t const *const>(box->bin) + box->accessors[2].offset + i*box->accessors[2].stride);
			vertex.pos = glm::vec4{*pos, 1.0f};
			vertex.normal = glm::vec4{*normal, 0.0f};
		#if 0
			vertex.pos.x = -vertex.pos.x;
			vertex.normal.x = -vertex.normal.x;
			vertex.pos.z = -vertex.pos.z;
			vertex.normal.z = -vertex.normal.z;
		#endif
			dprint("\n{},{},{} {},{},{}", vertex.pos.x, vertex.pos.y, vertex.pos.z, vertex.normal.x, vertex.normal.y, vertex.normal.z);
			vertices.push_back(vertex);
		}

		vk::Buffer vk_vertex_staging_buffer = vk_device.createBuffer({
			vk::BufferCreateFlags{},
			sizeof(vertices[0])*vertices.size(),
			vk::BufferUsageFlagBits::eTransferSrc,
		});
		vk::MemoryRequirements vk_vertex_staging_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_vertex_staging_buffer);
		vk::DeviceMemory vk_vertex_staging_buffer_memory = vk_device.allocateMemory({
			vk_vertex_staging_buffer_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_vertex_staging_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent
			),
		});
		vk_device.bindBufferMemory(vk_vertex_staging_buffer, vk_vertex_staging_buffer_memory, 0);
		vk_map_memory(vk_device, vk_vertex_staging_buffer_memory, vertices);

		vk::Buffer vk_index_staging_buffer = vk_device.createBuffer({
			vk::BufferCreateFlags{},
			box_index_buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
		});
		vk::MemoryRequirements vk_index_staging_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_index_staging_buffer);
		vk::DeviceMemory vk_index_staging_buffer_memory = vk_device.allocateMemory({
			vk_index_staging_buffer_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_index_staging_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent
			),
		});
		vk_device.bindBufferMemory(vk_index_staging_buffer, vk_index_staging_buffer_memory, 0);
		{
			size_t index_buffer_offset = box->accessors[0].buffer_view->offset;
			size_t index_buffer_size = box->accessors[0].buffer_view->size;
			auto index_buffer_data = reinterpret_cast<uint8_t const *const>(box->bin) + index_buffer_offset;
			vk_map_memory(vk_device, vk_index_staging_buffer_memory, index_buffer_data, index_buffer_size);
		}

		vk::Sampler vk_sampler = vk_device.createSampler({
			// TODO
		});

		// NOTE: I am writing this code assuming that there exists a memory type index with memory properties host visible, host coherent and device local. This is not necessarily always the case!
		vk::Buffer vk_uniform_buffer = vk_device.createBuffer({
			vk::BufferCreateFlags{},
			sizeof(Uniforms),
			vk::BufferUsageFlagBits::eUniformBuffer,
		});
		vk::MemoryRequirements vk_uniform_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_uniform_buffer);
		vk::DeviceMemory vk_uniform_buffer_memory = vk_device.allocateMemory({
			vk_uniform_buffer_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_uniform_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal|
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent
			),
		});
		vk_device.bindBufferMemory(vk_uniform_buffer, vk_uniform_buffer_memory, 0);

		Uniforms uniforms;
		uniforms.model = box_model;
		uniforms.view = glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, 3.0f});
		uniforms.proj = perspective(aspect_ratio);
		vk_map_memory(vk_device, vk_uniform_buffer_memory, &uniforms, sizeof(uniforms));

		vk::Buffer vk_vertex_buffer = vk_device.createBuffer({
			vk::BufferCreateFlags{},
			box_vertex_buffer_size,
			vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
		});
		vk::MemoryRequirements vk_vertex_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_vertex_buffer);
		vk::DeviceMemory vk_vertex_buffer_memory = vk_device.allocateMemory({
			vk_vertex_buffer_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_vertex_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			),
		});
		vk_device.bindBufferMemory(vk_vertex_buffer, vk_vertex_buffer_memory, 0);

		vk::Buffer vk_index_buffer = vk_device.createBuffer({
			vk::BufferCreateFlags{},
			box_index_buffer_size,
			vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eIndexBuffer,
		});
		vk::MemoryRequirements vk_index_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_index_buffer);
		vk::DeviceMemory vk_index_buffer_memory = vk_device.allocateMemory({
			vk_index_buffer_memory_requirements.size,
			vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_index_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			),
		});
		vk_device.bindBufferMemory(vk_index_buffer, vk_index_buffer_memory, 0);


		// TODO: Implement descriptor indexing.
	#if 0
		std::array<vk::DescriptorSetLayoutBinding, 1> vk_descriptor_set_layout_binding_uniform_buffer{
			vk::DescriptorSetLayoutBinding{
				0,
				vk::DescriptorType::eUniformBuffer,
				1,
				vk::ShaderStageFlagBits::eVertex,
			},
		};
		std::array<vk::DescriptorSetLayoutBinding, 1> vk_descriptor_set_layout_binding_combined_image_sampler{
			vk::DescriptorSetLayoutBinding{
				0,
				vk::DescriptorType::eCombinedImageSampler,
				1,
				vk::ShaderStageFlagBits::eFragment,
			},
		};

		std::array<vk::DescriptorSetLayout, 2> vk_descriptor_set_layouts{};
	    vk_descriptor_set_layouts[0] = vk_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
	    	vk::DescriptorSetLayoutCreateFlags{},
	    	vk_descriptor_set_layout_binding_uniform_buffer,
	    });
	    vk_descriptor_set_layouts[1] = vk_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
	    	vk::DescriptorSetLayoutCreateFlags{},
	    	vk_descriptor_set_layout_binding_combined_image_sampler,
	    });

	    std::array<vk::DescriptorPoolSize, 2> vk_descriptor_pool_sizes{
	    	vk::DescriptorPoolSize{
	    		vk::DescriptorType::eUniformBuffer,
	    		1,
	    	},
	    	vk::DescriptorPoolSize{
	    		vk::DescriptorType::eCombinedImageSampler,
	    		1,
	    	},
	    };

	    vk::DescriptorPool vk_descriptor_pool = vk_device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
	    	vk::DescriptorPoolCreateFlags{},
	    	static_cast<uint32_t>(vk_descriptor_pool_sizes.size()),
	    	vk_descriptor_pool_sizes,
	    });

	    std::vector<vk::DescriptorSet> vk_descriptor_sets = vk_device.allocateDescriptorSets({
	    	vk_descriptor_pool,
	    	vk_descriptor_set_layouts,
	    });

	    std::array<vk::DescriptorBufferInfo, 1> vk_descriptor_buffer_infos{
	    	vk::DescriptorBufferInfo{
	    		vk_uniform_buffer,
	    		0,
	    		sizeof(Uniforms),
	    	},
	    };

	    std::array<vk::DescriptorImageInfo, 1> vk_descriptor_image_infos{
	    	vk::DescriptorImageInfo{
	    		vk_sampler,
	    		// vk_image_view, TODO
	    		// vk::ImageLayout::eShaderReadOnlyOptimal, TODO
	    	},
	    };    

	    std::array<vk::WriteDescriptorSet, 2> vk_descriptor_writes{
	    	vk::WriteDescriptorSet{
	    		vk_descriptor_sets[0],
	    		0, 0,
	    		vk::DescriptorType::eUniformBuffer,
	    		{},
	    		vk_descriptor_buffer_infos,
	    		{},
	    	},
	    	vk::WriteDescriptorSet{
	    		vk_descriptor_sets[1],
	    		0, 0,
	    		vk::DescriptorType::eCombinedImageSampler,
	    		vk_descriptor_image_infos,
	    		{},
	    		{},
	    	},
	    };
	#endif

		std::array<vk::DescriptorSetLayoutBinding, 1> vk_descriptor_set_layout_binding_uniform_buffer{
			vk::DescriptorSetLayoutBinding{
				0,
				vk::DescriptorType::eUniformBuffer,
				1,
				vk::ShaderStageFlagBits::eVertex,
			},
		};

		std::array<vk::DescriptorSetLayout, 1> vk_descriptor_set_layouts{};
	    vk_descriptor_set_layouts[0] = vk_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
	    	vk::DescriptorSetLayoutCreateFlags{},
	    	vk_descriptor_set_layout_binding_uniform_buffer,
	    });

	    std::array<vk::DescriptorPoolSize, 1> vk_descriptor_pool_sizes{
	    	vk::DescriptorPoolSize{
	    		vk::DescriptorType::eUniformBuffer,
	    		1,
	    	},
	    };

	    vk::DescriptorPool vk_descriptor_pool = vk_device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
	    	vk::DescriptorPoolCreateFlags{},
	    	static_cast<uint32_t>(vk_descriptor_pool_sizes.size()),
	    	vk_descriptor_pool_sizes,
	    });

	    std::vector<vk::DescriptorSet> vk_descriptor_sets = vk_device.allocateDescriptorSets({
	    	vk_descriptor_pool,
	    	vk_descriptor_set_layouts,
	    });

	    std::array<vk::DescriptorBufferInfo, 1> vk_descriptor_buffer_infos{
	    	vk::DescriptorBufferInfo{
	    		vk_uniform_buffer,
	    		0,
	    		sizeof(Uniforms),
	    	},
	    };

	    std::array<vk::WriteDescriptorSet, 1> vk_descriptor_writes{
	    	vk::WriteDescriptorSet{
	    		vk_descriptor_sets[0],
	    		0, 0,
	    		vk::DescriptorType::eUniformBuffer,
	    		{},
	    		vk_descriptor_buffer_infos,
	    		{},
	    	},
	    };

	    vk_device.updateDescriptorSets(vk_descriptor_writes, {});

	    vk::PipelineLayout vk_pipeline_layout = vk_device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
	    	vk::PipelineLayoutCreateFlags{},
	    	vk_descriptor_set_layouts,
	    });

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

			std::array<char const *, 1> const search_paths{
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

		vk::PipelineCacheCreateFlagBits vk_pipeline_cache_flag_bits{};
		if (std::get<3>(vk_physical_device_features).pipelineCreationCacheControl)
		{
			vk_pipeline_cache_flag_bits = vk::PipelineCacheCreateFlagBits::eExternallySynchronized;
		}
		vk::PipelineCache vk_pipeline_cache = vk_device.createPipelineCache(
			{vk_pipeline_cache_flag_bits}
		);

		Slang::ComPtr<slang::IModule> slang_module;
		Slang::ComPtr<slang::IBlob> slang_module_diagnostics;
		slang_module = slang_session->loadModule("box", slang_module_diagnostics.writeRef());
		if (slang_module_diagnostics.get())
		{
			// TODO: Find a way to get shader compile errors in the Sublime Text console.
			throw std::runtime_error(
				FORMAT_ERROR(std::string_view(
					static_cast<char const *>(slang_module_diagnostics->getBufferPointer()),
					static_cast<size_t>(slang_module_diagnostics->getBufferSize())
				))
			);
		}

		Slang::ComPtr<slang::IBlob> slang_spirv_code_vs;
		Slang::ComPtr<slang::IEntryPoint> slang_entry_point_vs;
		Slang::ComPtr<slang::IComponentType> slang_composed_program_vs;
		Slang::ComPtr<slang::IComponentType> slang_linked_program_vs;
		{
			using namespace Slang;
			using namespace slang;

			SLANG_CHECK(slang_module->findEntryPointByName("vs", slang_entry_point_vs.writeRef()));

			std::array<IComponentType *, 2> component_types{
				slang_module,
				slang_entry_point_vs,
			};
			SLANG_CHECK(slang_session->createCompositeComponentType(
				component_types.data(),
				component_types.size(),
				slang_composed_program_vs.writeRef()
			));

			SLANG_CHECK(slang_composed_program_vs->link(slang_linked_program_vs.writeRef()));

			SLANG_CHECK(slang_linked_program_vs->getEntryPointCode(
				0, // entryPointIndex
				0, // targetIndex
				slang_spirv_code_vs.writeRef()
			));
		}
		vk::ShaderModule vk_vertex_shader_module = vk_device.createShaderModule({
			{},
			static_cast<uint32_t>(slang_spirv_code_vs->getBufferSize()),
			static_cast<uint32_t const *>(slang_spirv_code_vs->getBufferPointer()),
		});
		vk::PipelineShaderStageCreateInfo vk_vertex_shader_stage_create_info{
			{},
			vk::ShaderStageFlagBits::eVertex,
			vk_vertex_shader_module,
			"main",
		};

		Slang::ComPtr<slang::IBlob> slang_spirv_code_ps;
		Slang::ComPtr<slang::IEntryPoint> slang_entry_point_ps;
		Slang::ComPtr<slang::IComponentType> slang_composed_program_ps;
		Slang::ComPtr<slang::IComponentType> slang_linked_program_ps;
		{
			using namespace Slang;
			using namespace slang;

			SLANG_CHECK(slang_module->findEntryPointByName("ps", slang_entry_point_ps.writeRef()));

			std::array<IComponentType *, 2> component_types{
				slang_module,
				slang_entry_point_ps,
			};
			SLANG_CHECK(slang_session->createCompositeComponentType(
				component_types.data(),
				component_types.size(),
				slang_composed_program_ps.writeRef()
			));

			SLANG_CHECK(slang_composed_program_ps->link(slang_linked_program_ps.writeRef()));

			SLANG_CHECK(slang_linked_program_ps->getEntryPointCode(
				0, // entryPointIndex
				0, // targetIndex
				slang_spirv_code_ps.writeRef()
			));
		}
		vk::ShaderModule vk_fragment_shader_module = vk_device.createShaderModule({
			{},
			static_cast<uint32_t>(slang_spirv_code_ps->getBufferSize()),
			static_cast<uint32_t const *>(slang_spirv_code_ps->getBufferPointer()),
		});
		vk::PipelineShaderStageCreateInfo vk_fragment_shader_stage_create_info{
			{},
			vk::ShaderStageFlagBits::eFragment,
			vk_fragment_shader_module,
			"main",
		};

		std::array<vk::PipelineShaderStageCreateInfo, 2> vk_shader_stage_create_infos{
			vk_vertex_shader_stage_create_info,
			vk_fragment_shader_stage_create_info,
		};

		std::array<vk::VertexInputBindingDescription, 1> vk_vertex_input_binding_descriptions{
			vk::VertexInputBindingDescription{
				0,
				sizeof(Vertex),
				vk::VertexInputRate::eVertex,
			}
		};

		std::array<vk::VertexInputAttributeDescription, 2> vk_vertex_input_attribute_descriptions{
			vk::VertexInputAttributeDescription{
				0,
				0,
				vk::Format::eR32G32B32A32Sfloat,
				offsetof(Vertex, pos),
			},
			vk::VertexInputAttributeDescription{
				1,
				0,
				vk::Format::eR32G32B32A32Sfloat,
				offsetof(Vertex, normal),
			},
		};

		vk::PipelineVertexInputStateCreateInfo vk_vertex_input_state_create_info{
			vk::PipelineVertexInputStateCreateFlags{},
			vk_vertex_input_binding_descriptions,
			vk_vertex_input_attribute_descriptions,
		};

		vk::PipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{
			{},
			vk::PrimitiveTopology::eTriangleList,
		};

		std::array<vk::Viewport, 1> vk_viewports{
			vk::Viewport{
				0.0f,
				0.0f,
				static_cast<float>(vk_swapchain_extent.width),
				static_cast<float>(vk_swapchain_extent.height),
				0.0f,
				1.0f,
			},
		};

		std::array<vk::Rect2D, 1> vk_scissors{
			vk::Rect2D{
				vk::Offset2D{0, 0},
				vk_swapchain_extent,
			},
		};

		vk::PipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info{
			vk::PipelineViewportStateCreateFlags{},
			vk_viewports,
			vk_scissors,
		};

		vk::PipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{
			vk::PipelineRasterizationStateCreateFlags{},
			vk::False,
			vk::False,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			vk::False,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
		};
		vk::PipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info{};

		vk::PipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info{
			vk::PipelineDepthStencilStateCreateFlags{},
			vk::False,
			vk::False,
			vk::CompareOp::eGreater,
			vk::False,
			vk::False,
		};

		std::array<vk::PipelineColorBlendAttachmentState, 1> vk_pipeline_color_blend_attachment_states{
			vk::PipelineColorBlendAttachmentState{
				vk::False,
				vk::BlendFactor::eZero,
				vk::BlendFactor::eZero,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eZero,
				vk::BlendFactor::eZero,
				vk::BlendOp::eAdd,
				vk::ColorComponentFlagBits::eR|
				vk::ColorComponentFlagBits::eG|
				vk::ColorComponentFlagBits::eB|
				vk::ColorComponentFlagBits::eA,
			},
		};

		vk::PipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{
			vk::PipelineColorBlendStateCreateFlags{},
			{},
			vk::LogicOp::eClear,
			vk_pipeline_color_blend_attachment_states,
		};

		vk::PipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info{};

		std::array<vk::Format const, 1> const vk_color_attachment_formats{
			vk_swapchain_format,
		};

		vk::PipelineRenderingCreateInfo vk_pipeline_rendering_create_info{
			0,
			vk_color_attachment_formats,
			vk_depth_format,
			// vk_stencil_format,
		};

		vk::GraphicsPipelineCreateInfo vk_graphics_pipeline_create_info{
	#if BASED_RENDERER_VK_DISABLE_PIPELINE_OPTIMIZATION
			vk::PipelineCreateFlagBits::eDisableOptimization,
	#else
			{},
	#endif
			vk_shader_stage_create_infos,
			&vk_vertex_input_state_create_info,
			&vk_pipeline_input_assembly_state_create_info,
			nullptr,
			&vk_pipeline_viewport_state_create_info,
			&vk_pipeline_rasterization_state_create_info,
			&vk_pipeline_multisample_state_create_info,
			&vk_pipeline_depth_stencil_state_create_info,
			&vk_pipeline_color_blend_state_create_info,
			&vk_pipeline_dynamic_state_create_info,
			vk_pipeline_layout,
			{},
			{},
			{},
			{},
			&vk_pipeline_rendering_create_info,
		};

		auto vk_pipelines = *vk_device.createGraphicsPipelines(
			vk_pipeline_cache,
			{
				vk_graphics_pipeline_create_info
			}
		);

		size_t vk_frame_idx = 0;

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
			
			vk::detail::resultCheck(vk_device.waitForFences(
				{vk_fences[vk_frame_idx]}, 
				vk::True, 
				std::numeric_limits<uint64_t>::max()), "Failed to wait for fence.");
			vk_device.resetFences({vk_fences[vk_frame_idx]});

			uint32_t vk_image_idx = *vk_device.acquireNextImageKHR(
				vk_swapchain, 
				std::numeric_limits<uint64_t>::max(), 
				vk_semaphores_wait[vk_frame_idx]
			);

			// We only show the window once we've arrived back at the first frame.
			// This only makes sense if there are just two frames, that is, one backbuffer
			// and one frontbuffer. Which is to say, it is to be thrown away!
			if (vk_image_idx == 0) 
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

			UNUSED(fixed_dt);

	#if 0
			update_cube(vk_device, vk_uniform_buffer_memory, uniforms, fixed_dt);
	#endif

			vk::CommandBuffer cb = vk_graphics_command_buffers[vk_frame_idx];
			cb.begin({
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			});

			// It might seem unnecessarily verbose that I am doing things this way. Why not just create the array once, and then modify it based on the value of "staged"? In my experience, I have found that when I am not this explicit about things in Vulkan, it makes it a lot harder to find mistakes.
			static size_t staged = 0;
			if (staged == 0)
			{
				std::array<vk::BufferMemoryBarrier2, 4> buffer_barriers{
					vk::BufferMemoryBarrier2{
						vk::PipelineStageFlags2{},
						vk::AccessFlags2{},
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferRead,
						0,
						0,
						vk_vertex_staging_buffer,
						0,
						box_vertex_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						vk::PipelineStageFlags2{},
						vk::AccessFlags2{},
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferRead,
						0,
						0,
						vk_index_staging_buffer,
						0,
						box_index_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						vk::PipelineStageFlags2{},
						vk::AccessFlags2{},
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						0,
						0,
						vk_vertex_buffer,
						0,
						box_vertex_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						vk::PipelineStageFlags2{},
						vk::AccessFlags2{},
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						0,
						0,
						vk_index_buffer,
						0,
						box_index_buffer_size,
					},
				};

				// TODO: Does it really make sense to just shove as much as possible into one call to pipelineBarrier2? I feel like it probably doesn't. These images don't have to get transitioned at this point. They could be transitioned after copying the buffers.
				std::array<vk::ImageMemoryBarrier2, 2> image_barriers{
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{},
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eColorAttachmentOptimal,
						0, // TODO: srcQueueFamilyIdx
						0, // TODO: dstQueueFamilyIdx
						vk_swapchain_images[vk_image_idx],
						vk::ImageSubresourceRange{
							vk::ImageAspectFlags{vk::ImageAspectFlagBits::eColor},
							0,
							1,
							0,
							1,
						},
					},
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
						vk::AccessFlags2{},
						vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
						vk::AccessFlags2{vk::AccessFlagBits2::eDepthStencilAttachmentWrite},
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eDepthAttachmentOptimal,
						0, // TODO: srcQueueFamilyIdx
						0, // TODO: dstQueueFamilyIdx
						vk_depth_image,
						vk::ImageSubresourceRange{
							vk::ImageAspectFlags{vk::ImageAspectFlagBits::eDepth},
							0,
							1,
							0,
							1,
						},
					},
	#if 0
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlagBits2{},
						vk::AccessFlagBits2{},
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eTransferDstOptimal,
						0, // TODO: srcQueueFamilyIdx
						0, // TODO: dstQueueFamilyIdx
						vk_image,
						vk::ImageSubresourceRange{
							vk::ImageAspectFlags{vk::ImageAspectFlagBits::eColor},
							0,
							1,
							0,
							1,
						},
					},
	#endif
				};

				cb.pipelineBarrier2({
					vk::DependencyFlags{},
					{},
					buffer_barriers,
					image_barriers,
				});

				std::array<vk::BufferCopy, 1> vertex_buffer_regions{
					vk::BufferCopy{
						0,
						0,
						box_vertex_buffer_size,
					},
				};

				cb.copyBuffer(vk_vertex_staging_buffer, vk_vertex_buffer, vertex_buffer_regions);

				std::array<vk::BufferCopy, 1> index_buffer_regions{
					vk::BufferCopy{
						0,
						0,
						box_index_buffer_size,
					},
				};

				cb.copyBuffer(vk_index_staging_buffer, vk_index_buffer, index_buffer_regions);

				std::array<vk::BufferMemoryBarrier2, 2> buffer_barriers2{
					vk::BufferMemoryBarrier2{
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						vk::PipelineStageFlagBits2::eVertexAttributeInput,
						vk::AccessFlagBits2::eVertexAttributeRead,
						0,
						0,
						vk_vertex_buffer,
						0,
						box_vertex_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						vk::PipelineStageFlagBits2::eIndexInput,
						vk::AccessFlagBits2::eIndexRead,
						0,
						0,
						vk_index_buffer,
						0,
						box_index_buffer_size,
					},
				};

				cb.pipelineBarrier2({
					vk::DependencyFlags{},
					{},
					buffer_barriers2,
					{},
				});

	#if 0
				std::array<vk::BufferImageCopy, 1> regions{
					vk::BufferImageCopy{
						0, 0, 0,
						vk::ImageSubresourceLayers{
							vk::ImageAspectFlagBits::eColor,
							0,
							0,
							1, 
						},
						vk::Offset3D{},
						vk::Extent3D{
							static_cast<uint32_t>(stone_image_width),
							static_cast<uint32_t>(stone_image_height),
							1
						},
					},
				};

				cb.copyBufferToImage(
					vk_staging_buffer,
					vk_image,
					vk::ImageLayout::eTransferDstOptimal,
					regions);


				std::array<vk::ImageMemoryBarrier2, 1> image_barriers2{
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						vk::PipelineStageFlagBits2::eFragmentShader,
						vk::AccessFlagBits2::eShaderSampledRead,
						vk::ImageLayout::eTransferDstOptimal,
						vk::ImageLayout::eShaderReadOnlyOptimal,
						0, // TODO: srcQueueFamilyIdx
						0, // TODO: dstQueueFamilyIdx
						vk_image,
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
					image_barriers2,
				});
	#endif

				staged += 1;
			}
			else if (staged < vk_swapchain_images.size())
			{
				std::array<vk::ImageMemoryBarrier2, 1> image_barriers{
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{},
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eColorAttachmentOptimal,
						0, // TODO: srcQueueFamilyIdx
						0, // TODO: dstQueueFamilyIdx
						vk_swapchain_images[vk_image_idx],
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
					image_barriers,
				});

				staged += 1;
			}
			else
			{
				std::array<vk::ImageMemoryBarrier2, 1> image_barriers{
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{},
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
						vk::ImageLayout::ePresentSrcKHR,
						vk::ImageLayout::eColorAttachmentOptimal,
						0, // TODO: srcQueueFamilyIdx
						0, // TODO: dstQueueFamilyIdx
						vk_swapchain_images[vk_image_idx],
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
					image_barriers,
				});
			}

			std::array<vk::RenderingAttachmentInfo, 1> vk_rendering_attachment_infos{
				vk::RenderingAttachmentInfo{
					vk_swapchain_image_views[vk_image_idx],
					vk::ImageLayout::eColorAttachmentOptimal,

					vk::ResolveModeFlagBits::eNone,
					vk::ImageView{},
					vk::ImageLayout::eUndefined,

					vk::AttachmentLoadOp::eClear,
					vk::AttachmentStoreOp::eStore,
					vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f),
				},
			};

			vk::RenderingAttachmentInfo vk_depth_attachment_info{
				vk_depth_image_view,
				vk::ImageLayout::eDepthAttachmentOptimal,

				vk::ResolveModeFlagBits::eNone,
				vk::ImageView{},
				vk::ImageLayout::eUndefined,

				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eDontCare,
				vk::ClearDepthStencilValue(0.0f, 0),
			};

			cb.beginRendering({
				vk::RenderingFlags{},
				vk::Rect2D{
					vk::Offset2D{0, 0},
					vk_swapchain_extent,
				},
				1,
				0,
				vk_rendering_attachment_infos,
				&vk_depth_attachment_info,
				//&vk_stencil_attachment_info,
			});

			cb.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				vk_pipelines[0]
			);
			std::array<vk::Buffer, 1> vertex_buffers{vk_vertex_buffer};
			std::array<vk::DeviceSize, 1> offsets{0};
			cb.bindVertexBuffers(0, vertex_buffers, offsets);
			cb.bindIndexBuffer(vk_index_buffer, 0, vk::IndexType::eUint16);
			cb.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				vk_pipeline_layout,
				0,
				vk_descriptor_sets,
				{}
			);
			cb.drawIndexed(static_cast<uint32_t>(box->accessors[0].count), 1, 0, 0, 0);

			cb.endRendering();

			{
				std::array<vk::ImageMemoryBarrier2, 1> image_barriers{
					vk::ImageMemoryBarrier2{
						vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
						vk::PipelineStageFlags2{},
						vk::AccessFlags2{},
						vk::ImageLayout::eColorAttachmentOptimal,
						vk::ImageLayout::ePresentSrcKHR,
						0, // TODO
						0, // TODO
						vk_swapchain_images[vk_image_idx],
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
					image_barriers,
				});
			}

			cb.end();

			std::array<vk::SemaphoreSubmitInfo, 1> vk_wait_semaphore_infos{
				vk::SemaphoreSubmitInfo{
					vk_semaphores_wait[vk_frame_idx],
					0,
					vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				},
			};

			std::array<vk::CommandBufferSubmitInfo, 1> vk_command_buffer_submit_infos{
				{
					cb,
				},
			};

			std::array<vk::SemaphoreSubmitInfo, 1> vk_signal_semaphore_infos{
				vk::SemaphoreSubmitInfo{
					vk_semaphores_signal[vk_frame_idx],
					0,
					vk::PipelineStageFlagBits2::eAllCommands, // This is needed, or else the present will start before all commands have finished.
				},
			};

			std::array<vk::SubmitInfo2, 1> vk_submit_infos{
				vk::SubmitInfo2{
					{},
					vk_wait_semaphore_infos,
					vk_command_buffer_submit_infos,
					vk_signal_semaphore_infos,
				}
			};
			vk_graphics_queue.submit2(vk_submit_infos, vk_fences[vk_frame_idx]);

			std::array<vk::Semaphore, 1> vk_present_wait_semaphores{vk_semaphores_signal[vk_frame_idx]};
			std::array<vk::SwapchainKHR, 1> vk_present_swapchains{vk_swapchain};
			std::array<uint32_t, 1> vk_present_image_indices{vk_image_idx};
			std::array<vk::Result, 1> vk_present_results;
			// TODO: Use the present queue.
			vk::detail::resultCheck(vk_graphics_queue.presentKHR({
				vk_present_wait_semaphores,
				vk_present_swapchains,
				vk_present_image_indices,
				vk_present_results
			}), "Failed to present.");
			vk::detail::resultCheck(vk_present_results[0], "Failed to present.");

			vk_frame_idx = (vk_frame_idx + 1) % vk_swapchain_images.size();
		}
	}
} // based_renderer