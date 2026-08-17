/*
Conventions:

- World space coordinate system: x=right, y=forward, z=up, right-handed
*/

#include "main_pch.hpp"

#define UNUSED(X) (void)(X)
#define STRINGIFY(x) #x
#define STMT(X) do {X} while (0)
#define FORMAT_ERROR(MESSAGE) std::format("{}({}): {}", __FUNCTION__, __LINE__, (MESSAGE))

#ifdef _DEBUG
#define BASED_RENDERER_DEBUG 1
#else
#define BASED_RENDERER_DEBUG 0
#endif

#define BASED_RENDERER_VK_DEBUG BASED_RENDERER_DEBUG
#define BASED_RENDERER_VK_DEBUG_OUTPUT BASED_RENDERER_VK_DEBUG
#define BASED_RENDERER_VK_DISABLE_PIPELINE_OPTIMIZATION BASED_RENDERER_VK_DEBUG

#define BASED_RENDERER_SLANG_DEBUG BASED_RENDERER_VK_DEBUG
#define BASED_RENDERER_SLANG_SPIRV_VALIDATION BASED_RENDERER_SLANG_DEBUG

#define BASED_RENDERER_FULLSCREEN 1

#define VK_KHR_platform_surface "VK_KHR_win32_surface"

namespace based_renderer
{
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

	static std::vector<char const *> vk_get_instance_extensions();
	static std::vector<char const *> vk_get_device_extensions(vk::PhysicalDevice const physical_device);
	
	static std::vector<vk::DeviceQueueCreateInfo> vk_get_device_queue_infos(std::vector<vk::QueueFamilyProperties> const &queue_family_properties);
	static std::vector<std::vector<vk::Queue>> vk_get_queues(vk::Device const device, std::vector<vk::QueueFamilyProperties> const &queue_family_properties);
	static size_t vk_find_queue_family_idx(std::vector<vk::QueueFamilyProperties> const &queue_family_properties, vk::QueueFlagBits const flags);
	
	static void vk_map_memory(vk::Device const device, vk::DeviceMemory const device_memory, void const *memory, vk::DeviceSize const memory_size);
	
	template <typename T>
	static void vk_map_memory(vk::Device const device, vk::DeviceMemory const device_memory, std::vector<T> const &memory)
	{
		vk_map_memory(device, device_memory, memory.data(), sizeof(T)*memory.size());
	}
	
	static uint32_t vk_find_memory_type_idx(
		vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
		uint32_t const memory_type_bits,
		vk::MemoryPropertyFlags const required_memory_properties);

	static uint32_t vk_find_memory_type_idx(
		vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
		uint32_t const memory_type_bits,
		vk::MemoryPropertyFlags const required_memory_properties,
		vk::MemoryPropertyFlags const desired_memory_properties);

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

	static void main();
}

int WINAPI WinMain(
	HINSTANCE win32_instance,
	HINSTANCE win32_prev_instance,
	LPSTR win32_command_line,
	int	win32_show_command)
{
	UNUSED(win32_instance);
	UNUSED(win32_prev_instance);
	UNUSED(win32_command_line);
	UNUSED(win32_show_command);

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
	struct context_t
	{
		bool running;
		bool should_rotate = true;
		int32_t key_w;
		int32_t key_s;
		int32_t key_a;
		int32_t key_d;
		glm::ivec2 mouse_pos;

		glm::uvec2 const client_dimensions;
		float const aspect_ratio;
		float const fixed_dt;
	};

	LRESULT WINAPI win32_event_callback(
		HWND   win32_window,
		UINT   win32_message,
		WPARAM win32_w_param,
		LPARAM win32_l_param) noexcept
	{
		LRESULT res = 0;
		context_t &ctx = *reinterpret_cast<context_t *>(GetPropW(win32_window, L"based_renderer::context_t"));

		switch (win32_message)
		{
			case WM_DESTROY:
			case WM_CLOSE: 
			{
				ctx.running = false;
			} break;
			case WM_KEYDOWN: 
				{
				uint8_t virtual_key_code = static_cast<uint8_t>(win32_w_param);
				switch (virtual_key_code)
				{
					case VK_SPACE:
					{
						ctx.should_rotate = !ctx.should_rotate;
					} break;
					case 'W':
					{
						ctx.key_w = true;
					} break;
					case 'S':
					{
						ctx.key_s = true;
					} break;
					case 'A':
					{
						ctx.key_a = true;
					} break;
					case 'D':
					{
						ctx.key_d = true;
					} break;
					case VK_ESCAPE: 
					{
						ctx.running = false;
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
						ctx.key_w = false;
					} break;
					case 'S':
					{
						ctx.key_s = false;
					} break;
					case 'A':
					{
						ctx.key_a = false;
					} break;
					case 'D':
					{
						ctx.key_d = false;
					} break;
					case VK_ESCAPE: 
					{
						ctx.running = false;
					} break;
				}
			} break;
			case WM_MOUSEMOVE:
			{
				memcpy(&ctx.mouse_pos, &win32_l_param, sizeof(LPARAM));
			} break;
			default: 
			{
				res = DefWindowProcW(win32_window, win32_message, win32_w_param, win32_l_param);
			} break;
		}

		return res;
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

	struct uniforms_t
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	float constexpr TAU = 6.28318530717958647693f;

	// FGED Listing 6.4
	static glm::mat4 perspective(
		float const fov_y, 
		float const aspect_ratio, 
		float const near_plane, 
		float const epsilon) noexcept
	{
		float const proj_plane = 1.0f / tanf(fov_y * 0.5f);

		return glm::mat4{
			proj_plane / aspect_ratio, 0.0f, 0.0f, 0.0f,
			0.0f, proj_plane, 0.0f, 0.0f,
			0.0f, 0.0f, epsilon, near_plane * (1.0f - epsilon),
			0.0f, 0.0f, 1.0f, 0.0f,
		};
	}

	#if 1
	static void update_cube(
		context_t &ctx,
		vk::Device const device, 
		vk::DeviceMemory const uniforms_memory, 
		uniforms_t &uniforms)
	{
		if (ctx.should_rotate)
		{
			uniforms.model = glm::rotate(uniforms.model, ctx.fixed_dt, glm::normalize(glm::vec3{3.0f, 2.0f, 1.0f}));
		}

		// TODO: Inverting the view matrix should only require transposing the upper 3x3 matrix and negating the translation values.
		uniforms.view = glm::inverse(uniforms.view);

		// Rotate based on mouse delta (doesn't work at all right now).
		// TODO: Add mouse delta to ctx.
		static glm::ivec2 last_mouse_pos{-1, -1};
		if (last_mouse_pos == glm::ivec2{-1, -1})
		{
			last_mouse_pos = ctx.mouse_pos;
		}
		glm::vec2 mouse_pos_diff = glm::vec2{ctx.mouse_pos - last_mouse_pos};

		glm::vec3 angle{mouse_pos_diff.x*ctx.fixed_dt/(TAU*2048.0f), 0.0f, mouse_pos_diff.y*ctx.fixed_dt/(TAU*2048.0f)};
		uniforms.view = glm::rotateNormalizedAxis(uniforms.view, glm::length(angle), glm::normalize(angle));
		last_mouse_pos = ctx.mouse_pos;

		// Translate based on whether WASD keys are pressed.
		int32_t const cube_dir_z = ctx.key_s - ctx.key_w;
		int32_t const cube_dir_x = ctx.key_a - ctx.key_d;
		uniforms.view = glm::translate(uniforms.view, glm::vec3{static_cast<float>(cube_dir_x)*ctx.fixed_dt, 0.0f, static_cast<float>(cube_dir_z)*ctx.fixed_dt});

		uniforms.view = glm::inverse(uniforms.view);

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
			throw std::runtime_error{std::format("{}({}): Failed to load {}.", __FUNCTION__, __LINE__, path)};
		}

		return data;
	}

	struct buffer_info_t
	{
    	void const *data;
		vk::DeviceSize const size; // in bytes
    	vk::BufferUsageFlags const usage;
    	std::vector<vk::VertexInputBindingDescription2EXT> bindings;
    	std::vector<vk::VertexInputAttributeDescription2EXT> attributes;
	};

	struct gltf_process_data_result_t
	{
		std::vector<buffer_info_t> buffer_infos;
	};

	static vk::Format vk_format(cgltf_type const type, cgltf_component_type const component_type) noexcept
	{
		if (type == cgltf_type_invalid || component_type == cgltf_component_type_invalid)
		{
			return vk::Format::eUndefined;
		}

		switch (type)
		{
			case cgltf_type_scalar:
				switch (component_type)
				{
					case cgltf_component_type_r_8:
						return vk::Format::eR8Sint;
					case cgltf_component_type_r_8u:
						return vk::Format::eR8Uint;
					case cgltf_component_type_r_16:
						return vk::Format::eR16Sint;
					case cgltf_component_type_r_16u:
						return vk::Format::eR16Uint;
					case cgltf_component_type_r_32u:
						return vk::Format::eR32Uint;
					case cgltf_component_type_r_32f:
						return vk::Format::eR32Sfloat;
				}
			case cgltf_type_vec2:
			case cgltf_type_mat2:
				switch (component_type)
				{
					case cgltf_component_type_r_8:
						return vk::Format::eR8G8Sint;
					case cgltf_component_type_r_8u:
						return vk::Format::eR8G8Uint;
					case cgltf_component_type_r_16:
						return vk::Format::eR16G16Sint;
					case cgltf_component_type_r_16u:
						return vk::Format::eR16G16Uint;
					case cgltf_component_type_r_32u:
						return vk::Format::eR32G32Uint;
					case cgltf_component_type_r_32f:
						return vk::Format::eR32G32Sfloat;
				}
			case cgltf_type_vec3:
			case cgltf_type_mat3:
				switch (component_type)
				{
					case cgltf_component_type_r_8:
						return vk::Format::eR8G8B8Sint;
					case cgltf_component_type_r_8u:
						return vk::Format::eR8G8B8Uint;
					case cgltf_component_type_r_16:
						return vk::Format::eR16G16B16Sint;
					case cgltf_component_type_r_16u:
						return vk::Format::eR16G16B16Uint;
					case cgltf_component_type_r_32u:
						return vk::Format::eR32G32B32Uint;
					case cgltf_component_type_r_32f:
						return vk::Format::eR32G32B32Sfloat;
				}
			case cgltf_type_vec4:
			case cgltf_type_mat4:
				switch (component_type)
				{
					case cgltf_component_type_r_8:
						return vk::Format::eR8G8B8A8Sint;
					case cgltf_component_type_r_8u:
						return vk::Format::eR8G8B8A8Uint;
					case cgltf_component_type_r_16:
						return vk::Format::eR16G16B16A16Sint;
					case cgltf_component_type_r_16u:
						return vk::Format::eR16G16B16A16Uint;
					case cgltf_component_type_r_32u:
						return vk::Format::eR32G32B32A32Uint;
					case cgltf_component_type_r_32f:
						return vk::Format::eR32G32B32A32Sfloat;
				}
		}

		return vk::Format::eUndefined;
	}

	// TODO: Would it make sense to make this into two functions: gltf_process_root_node and gltf_process_child_node?
	static void gltf_process_node(cgltf_node *node)
	{
		for (size_t child_idx = 0; child_idx < node->children_count; ++child_idx)
		{
			gltf_process_node(node->children[child_idx]);
		}

		// TODO: Process skin.

		// Process mesh.
		{
			cgltf_mesh *mesh = node->mesh;
		}

		// TODO: Process camera.
	}

	#define PTR_ADD(PTR, AMOUNT) (reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(PTR) + AMOUNT))

	/*
	Things we must be able to infer from the gltf data (# means I already know how to do it; ~ means I don't, but it doesn't matter yet):
	# Which camera to use.
	~ Which shaders do use.
	# Vertex input.
	# Input assembly.
	~ Tessellation state.
	~ Rasterization state.
	~ Multisample state.
	*/

	static gltf_process_data_result_t gltf_process_data(cgltf_data const *data)
	{
		gltf_process_data_result_t res{};

		for (size_t mesh_idx = 0; mesh_idx < data->meshes_count; ++mesh_idx)
		{
			cgltf_mesh *mesh = &data->meshs[mesh_idx];
			for (size_t primitive_idx = 0; primitive_idx < mesh->primitives_count; ++primitive_idx)
			{
				cgltf_primitive *primitive = &mesh->primitives[primitive_idx];
				


			}

			buffer_info_t buffer_create_info{
				.data = PTR_ADD(accessor->buffer_view->buffer->data, accessor->offset + accessor->buffer_view->offset),
				.size = (accessor->buffer_view->stride != 0 ? accessor->buffer_view->stride : accessor->stride) * accessor->count, // TODO: Is this correct?
				.usage = vk::BufferUsageFlagBits::eTransferDst|(accessor->buffer_view->type == cgltf_buffer_view_type_vertices ? vk::BufferUsageFlagBits::eVertexBuffer : BufferUsageFlagBits::eIndexBuffer),
			};
		}

		// for (size_t accessor_idx = 0; accessor_idx < data->accessors_count; ++accessor_idx)
		// {
		// 	cgltf_accessor *accessor = &data->accessors[accessor_idx];

		// 	buffer_info_t buffer_create_info{
		// 		.data = PTR_ADD(accessor->buffer_view->buffer->data, accessor->offset + accessor->buffer_view->offset),
		// 		.size = (accessor->buffer_view->stride != 0 ? accessor->buffer_view->stride : accessor->stride) * accessor->count, // TODO: Is this correct?
		// 		.usage = vk::BufferUsageFlagBits::eTransferDst|(accessor->buffer_view->type == cgltf_buffer_view_type_vertices ? vk::BufferUsageFlagBits::eVertexBuffer : BufferUsageFlagBits::eIndexBuffer),
		// 	};
		// }

		// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#concepts
		for (size_t scene_idx = 0; scene_idx < data->scenes_count; ++scene_idx)
		{
			cgltf_scene *scene = &data->scenes[scene_idx];
			
			for (size_t node_idx = 0; node_idx < scene->nodes_count; ++node_idx)
			{
				gltf_process_node(scene->nodes[node_idx]);
			}

			// TODO: Process extras.

			for (size_t extension_idx = 0; extension_idx < scene->extensions_count; ++extension_idx)
			{
				cgltf_extension *extension = &scene->extensions[extension_idx];
				// TODO: Process extensions.
				UNUSED(extension);
			}
		}

		return res;
	}

	#if 0
	static glm::mat4 gltf_node_transform_local(cgltf_node const *node) noexcept
	{
		glm::mat4 res;
		cgltf_node_transform_local(node, reinterpret_cast<float *>(&res));
		return res;
	}
	#else
	// static glm::mat4 gltf_node_transform_world(cgltf_node const *node) noexcept
	// {
	// 	glm::mat4 res;
	// 	cgltf_node_transform_world(node, reinterpret_cast<float *>(&res));
	// 	return res;
	// }
	#endif

	static void vk_modify_physical_device_features(auto &physical_device_features)
	{
		std::vector<std::string> missing_features;
		#define REQUIRE_FEATURE(FEATURE) STMT( \
			if (!features.FEATURE) \
			{ \
				missing_features.push_back(STRINGIFY(FEATURE)); \
			} \
		)
		#define DISABLE_FEATURE(FEATURE) STMT( \
			features.FEATURE = vk::False; \
		)
		// You might say: this doesn't do anything. Why define it?
		// It's so that way, when scrolling through all of the Vulkan
		// features I might want to use, none of them are missing.
		#define ALLOW_FEATURE(FEATURE)

		{
			auto &features = std::get<0>(physical_device_features).features;
			DISABLE_FEATURE(robustBufferAccess);
			DISABLE_FEATURE(fullDrawIndexUint32);
			DISABLE_FEATURE(imageCubeArray);
			DISABLE_FEATURE(independentBlend);
			DISABLE_FEATURE(geometryShader);
			DISABLE_FEATURE(tessellationShader);
			DISABLE_FEATURE(sampleRateShading);
			DISABLE_FEATURE(dualSrcBlend);
			DISABLE_FEATURE(logicOp);
			DISABLE_FEATURE(multiDrawIndirect);
			DISABLE_FEATURE(drawIndirectFirstInstance);
			DISABLE_FEATURE(depthClamp);
			DISABLE_FEATURE(depthBiasClamp);
			DISABLE_FEATURE(fillModeNonSolid);
			DISABLE_FEATURE(depthBounds);
			DISABLE_FEATURE(wideLines);
			DISABLE_FEATURE(largePoints);
			DISABLE_FEATURE(alphaToOne);
			DISABLE_FEATURE(multiViewport);
			DISABLE_FEATURE(samplerAnisotropy);
			DISABLE_FEATURE(textureCompressionETC2);
			DISABLE_FEATURE(textureCompressionASTC_LDR);
			DISABLE_FEATURE(textureCompressionBC);
			DISABLE_FEATURE(occlusionQueryPrecise);
			DISABLE_FEATURE(pipelineStatisticsQuery);
			DISABLE_FEATURE(vertexPipelineStoresAndAtomics);
			DISABLE_FEATURE(fragmentStoresAndAtomics);
			DISABLE_FEATURE(shaderTessellationAndGeometryPointSize);
			DISABLE_FEATURE(shaderImageGatherExtended);
			DISABLE_FEATURE(shaderStorageImageExtendedFormats);
			DISABLE_FEATURE(shaderStorageImageMultisample);
			DISABLE_FEATURE(shaderStorageImageReadWithoutFormat);
			DISABLE_FEATURE(shaderStorageImageWriteWithoutFormat);
			DISABLE_FEATURE(shaderUniformBufferArrayDynamicIndexing);
			DISABLE_FEATURE(shaderSampledImageArrayDynamicIndexing);
			DISABLE_FEATURE(shaderStorageBufferArrayDynamicIndexing);
			DISABLE_FEATURE(shaderStorageImageArrayDynamicIndexing);
			DISABLE_FEATURE(shaderClipDistance);
			DISABLE_FEATURE(shaderCullDistance);
			DISABLE_FEATURE(shaderFloat64);
			DISABLE_FEATURE(shaderInt64);
			DISABLE_FEATURE(shaderInt16);
			DISABLE_FEATURE(shaderResourceResidency);
			DISABLE_FEATURE(shaderResourceMinLod);
			DISABLE_FEATURE(sparseBinding);
			DISABLE_FEATURE(sparseResidencyBuffer);
			DISABLE_FEATURE(sparseResidencyImage2D);
			DISABLE_FEATURE(sparseResidencyImage3D);
			DISABLE_FEATURE(sparseResidency2Samples);
			DISABLE_FEATURE(sparseResidency4Samples);
			DISABLE_FEATURE(sparseResidency8Samples);
			DISABLE_FEATURE(sparseResidency16Samples);
			DISABLE_FEATURE(sparseResidencyAliased);
			DISABLE_FEATURE(variableMultisampleRate);
			DISABLE_FEATURE(inheritedQueries);
		}
		{
			auto &features = std::get<1>(physical_device_features);
			DISABLE_FEATURE(storageBuffer16BitAccess);
			DISABLE_FEATURE(uniformAndStorageBuffer16BitAccess);
			DISABLE_FEATURE(storagePushConstant16);
			DISABLE_FEATURE(storageInputOutput16);
			DISABLE_FEATURE(multiview);
			DISABLE_FEATURE(multiviewGeometryShader);
			DISABLE_FEATURE(multiviewTessellationShader);
			DISABLE_FEATURE(variablePointersStorageBuffer);
			DISABLE_FEATURE(variablePointers);
			DISABLE_FEATURE(protectedMemory);
			DISABLE_FEATURE(samplerYcbcrConversion);
			REQUIRE_FEATURE(shaderDrawParameters); // Slang requires this.
		}
		{
			auto &features = std::get<2>(physical_device_features);
			DISABLE_FEATURE(samplerMirrorClampToEdge);
			DISABLE_FEATURE(drawIndirectCount);
			DISABLE_FEATURE(storageBuffer8BitAccess);
			DISABLE_FEATURE(uniformAndStorageBuffer8BitAccess);
			DISABLE_FEATURE(storagePushConstant8);
			DISABLE_FEATURE(shaderBufferInt64Atomics);
			DISABLE_FEATURE(shaderSharedInt64Atomics);
			DISABLE_FEATURE(shaderFloat16);
			DISABLE_FEATURE(shaderInt8);
			DISABLE_FEATURE(descriptorIndexing);
			DISABLE_FEATURE(shaderInputAttachmentArrayDynamicIndexing);
			DISABLE_FEATURE(shaderUniformTexelBufferArrayDynamicIndexing);
			DISABLE_FEATURE(shaderStorageTexelBufferArrayDynamicIndexing);
			DISABLE_FEATURE(shaderUniformBufferArrayNonUniformIndexing);
			DISABLE_FEATURE(shaderSampledImageArrayNonUniformIndexing);
			DISABLE_FEATURE(shaderStorageBufferArrayNonUniformIndexing);
			DISABLE_FEATURE(shaderStorageImageArrayNonUniformIndexing);
			DISABLE_FEATURE(shaderInputAttachmentArrayNonUniformIndexing);
			DISABLE_FEATURE(shaderUniformTexelBufferArrayNonUniformIndexing);
			DISABLE_FEATURE(shaderStorageTexelBufferArrayNonUniformIndexing);
			DISABLE_FEATURE(descriptorBindingUniformBufferUpdateAfterBind); // TODO: If I remember correctly, my laptop does not support this feature, but it supports basically all the other "UpdateAfterBind" features. Does this matter? Can I still put the uniform buffer in the same descriptor set as every other descriptor, as long as I don't update it after binding?
			DISABLE_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
			DISABLE_FEATURE(descriptorBindingStorageImageUpdateAfterBind);
			DISABLE_FEATURE(descriptorBindingStorageBufferUpdateAfterBind);
			DISABLE_FEATURE(descriptorBindingUniformTexelBufferUpdateAfterBind);
			DISABLE_FEATURE(descriptorBindingStorageTexelBufferUpdateAfterBind);
			DISABLE_FEATURE(descriptorBindingUpdateUnusedWhilePending);
			DISABLE_FEATURE(descriptorBindingPartiallyBound);
			DISABLE_FEATURE(descriptorBindingVariableDescriptorCount);
			DISABLE_FEATURE(runtimeDescriptorArray);
			DISABLE_FEATURE(samplerFilterMinmax);
			DISABLE_FEATURE(scalarBlockLayout);
			DISABLE_FEATURE(imagelessFramebuffer);
			DISABLE_FEATURE(uniformBufferStandardLayout);
			DISABLE_FEATURE(shaderSubgroupExtendedTypes);
			DISABLE_FEATURE(separateDepthStencilLayouts);
			DISABLE_FEATURE(hostQueryReset);
			DISABLE_FEATURE(timelineSemaphore);
			DISABLE_FEATURE(bufferDeviceAddress);
			DISABLE_FEATURE(bufferDeviceAddressCaptureReplay);
			DISABLE_FEATURE(bufferDeviceAddressMultiDevice);
			REQUIRE_FEATURE(vulkanMemoryModel); // TODO: Do we necessarily need these?
			REQUIRE_FEATURE(vulkanMemoryModelDeviceScope);
			REQUIRE_FEATURE(vulkanMemoryModelAvailabilityVisibilityChains);
			DISABLE_FEATURE(shaderOutputViewportIndex);
			DISABLE_FEATURE(shaderOutputLayer);
			DISABLE_FEATURE(subgroupBroadcastDynamicId);
		}
		{
			auto &features = std::get<3>(physical_device_features);
			DISABLE_FEATURE(robustImageAccess);
			DISABLE_FEATURE(inlineUniformBlock);
			DISABLE_FEATURE(descriptorBindingInlineUniformBlockUpdateAfterBind);
			ALLOW_FEATURE(pipelineCreationCacheControl);
			DISABLE_FEATURE(privateData);
			DISABLE_FEATURE(shaderDemoteToHelperInvocation);
			DISABLE_FEATURE(shaderTerminateInvocation);
			DISABLE_FEATURE(subgroupSizeControl);
			DISABLE_FEATURE(computeFullSubgroups);
			REQUIRE_FEATURE(synchronization2);
			DISABLE_FEATURE(textureCompressionASTC_HDR);
			DISABLE_FEATURE(shaderZeroInitializeWorkgroupMemory);
			REQUIRE_FEATURE(dynamicRendering);
			DISABLE_FEATURE(shaderIntegerDotProduct);
			DISABLE_FEATURE(maintenance4);
		}
		{
			auto &features = std::get<4>(physical_device_features);
			DISABLE_FEATURE(globalPriorityQuery);
			DISABLE_FEATURE(shaderSubgroupRotate);
			DISABLE_FEATURE(shaderSubgroupRotateClustered);
			DISABLE_FEATURE(shaderFloatControls2);
			DISABLE_FEATURE(shaderExpectAssume);
			DISABLE_FEATURE(rectangularLines);
			DISABLE_FEATURE(bresenhamLines);
			DISABLE_FEATURE(smoothLines);
			DISABLE_FEATURE(stippledRectangularLines);
			DISABLE_FEATURE(stippledBresenhamLines);
			DISABLE_FEATURE(stippledSmoothLines);
			DISABLE_FEATURE(vertexAttributeInstanceRateDivisor);
			DISABLE_FEATURE(vertexAttributeInstanceRateZeroDivisor);
			DISABLE_FEATURE(indexTypeUint8);
			DISABLE_FEATURE(dynamicRenderingLocalRead);
			DISABLE_FEATURE(maintenance5);
			DISABLE_FEATURE(maintenance6);
			DISABLE_FEATURE(pipelineProtectedAccess);
			DISABLE_FEATURE(pipelineRobustness);
			DISABLE_FEATURE(hostImageCopy);
			DISABLE_FEATURE(pushDescriptor);
		}

		if (missing_features.size() > 0)
		{
			throw vk::FeatureNotPresentError{FORMAT_ERROR(to_string(missing_features))};
		}

		#undef REQUIRE_FEATURE
		#undef DISABLE_FEATURE
		#undef ALLOW_FEATURE
	}

	static std::system_error win32_system_error() noexcept
	{
		std::error_code error_code{static_cast<int>(GetLastError()), std::system_category()};
		std::system_error system_error{error_code};
		return system_error;
	}

	static void main()
	{
		std::vector<char const *> vk_instance_extensions = vk_get_instance_extensions();

		vk::ApplicationInfo vk_app_info{
			.pApplicationName = "based_renderer",
			.applicationVersion = VK_API_VERSION_1_0,
			.pEngineName = "based_renderer",
			.engineVersion = VK_API_VERSION_1_0,
			.apiVersion = VK_API_VERSION_1_4,
		};

	#if BASED_RENDERER_VK_DEBUG_OUTPUT
		vk::DebugUtilsMessengerCreateInfoEXT vk_debug_output_info{
			.flags = vk::DebugUtilsMessengerCreateFlagsEXT{},
			.messageSeverity = {
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | 
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | 
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | 
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
			},
			.messageType = {
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
			},
			.pfnUserCallback = vk_debug_callback,
			.pUserData = nullptr,
		};
	#endif

		vk::InstanceCreateInfo vk_instance_create_info{
			.flags = {},
			.pApplicationInfo = &vk_app_info,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = static_cast<uint32_t>(vk_instance_extensions.size()),
			.ppEnabledExtensionNames = vk_instance_extensions.data(),
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
			.pNext = &std::get<0>(vk_physical_device_features),
			.flags = {},
			.queueCreateInfoCount = static_cast<uint32_t>(vk_device_queue_infos.size()),
			.pQueueCreateInfos = vk_device_queue_infos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(vk_device_extensions.size()),
			.ppEnabledExtensionNames = vk_device_extensions.data(),
		});

		// Each queue family gets its own std::vector, whether or not it has any queues.
		std::vector<std::vector<vk::Queue>> vk_queues = vk_get_queues(vk_device, vk_queue_family_properties);

		size_t const vk_graphics_queue_family_idx = vk_find_queue_family_idx(vk_queue_family_properties, vk::QueueFlagBits::eGraphics);
		vk::Queue const vk_graphics_queue = vk_queues[vk_graphics_queue_family_idx][0];
		
		size_t const vk_transfer_queue_family_idx = vk_find_queue_family_idx(vk_queue_family_properties, vk::QueueFlagBits::eTransfer);
		vk::Queue const vk_transfer_queue = vk_queues[vk_transfer_queue_family_idx][0];

		vk::CommandPool vk_graphics_command_pool = vk_device.createCommandPool({
			.flags = vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = static_cast<uint32_t>(vk_graphics_queue_family_idx),
		});

		vk::CommandPool vk_transfer_command_pool;
		if (vk_graphics_queue_family_idx != vk_transfer_queue_family_idx)
		{
			vk_transfer_command_pool = vk_device.createCommandPool({
				.flags = vk::CommandPoolCreateFlags{},
				.queueFamilyIndex = static_cast<uint32_t>(vk_transfer_queue_family_idx),
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

		HINSTANCE const win32_instance = GetModuleHandleW(nullptr);

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

	    DEVMODEW win32_dev_mode = DEVMODEW{sizeof(DEVMODEW)};
	    if (!EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &win32_dev_mode))
	    {
	    	throw win32_system_error();
	    }

	    context_t ctx{
	    	.client_dimensions = glm::uvec2{
	    		static_cast<uint32_t>(win32_client_rect.right - win32_client_rect.left),
	    		static_cast<uint32_t>(win32_client_rect.bottom - win32_client_rect.top),
	    	},
	    	.aspect_ratio = static_cast<float>(win32_client_rect.right - win32_client_rect.left)/static_cast<float>(win32_client_rect.bottom - win32_client_rect.top),
	    	.fixed_dt = 1.0f/static_cast<float>(win32_dev_mode.dmDisplayFrequency),
	    };
	    SetPropW(win32_window, L"based_renderer::context_t", &ctx);

		vk::SurfaceKHR vk_surface = vk_instance.createWin32SurfaceKHR({
			.flags = {},
			.hinstance = win32_instance,
			.hwnd = win32_window,
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
			ctx.client_dimensions.x, 
			vk_surface_capabilities.minImageExtent.width, 
			vk_surface_capabilities.maxImageExtent.width
		);
		vk_swapchain_extent.height = std::clamp(
			ctx.client_dimensions.y, 
			vk_surface_capabilities.minImageExtent.height, 
			vk_surface_capabilities.maxImageExtent.height
		);

		vk::PresentModeKHR vk_swapchain_present_mode = vk::PresentModeKHR::eFifo; // NOTE: This is always supported.

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
			.flags = vk::SwapchainCreateFlagsKHR(),
			.surface = vk_surface,
			.minImageCount = 2,
			.imageFormat = vk_swapchain_format,
			.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
			.imageExtent = vk_swapchain_extent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = vk_pre_transform,
			.compositeAlpha = vk_composite_alpha,
			.presentMode = vk_swapchain_present_mode,
			.clipped = true,
			.oldSwapchain = nullptr,
		};

		vk::SwapchainKHR vk_swapchain = vk_device.createSwapchainKHR(vk_swapchain_create_info);

		std::vector<vk::Image> vk_swapchain_images = vk_device.getSwapchainImagesKHR(vk_swapchain);

		std::vector<vk::ImageView> vk_swapchain_image_views;
		vk_swapchain_image_views.reserve(vk_swapchain_images.size());
		vk::ImageViewCreateInfo vk_image_view_create_info{
			.flags = vk::ImageViewCreateFlags{}, 
			.image = nullptr,
			.viewType = vk::ImageViewType::e2D, 
			.format = vk_swapchain_format, 
			.components = vk::ComponentMapping{}, 
			.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
		};
		for (vk::Image image : vk_swapchain_images)
		{
			vk_image_view_create_info.image = image;
			vk_swapchain_image_views.push_back(vk_device.createImageView(vk_image_view_create_info));
		}

		std::vector<vk::CommandBuffer> vk_graphics_command_buffers = vk_device.allocateCommandBuffers({
			.commandPool = vk_graphics_command_pool, 
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = static_cast<uint32_t>(vk_swapchain_images.size()),
		});

		vk::CommandBuffer vk_transfer_command_buffer;
		if (vk_graphics_command_pool != vk_transfer_command_pool)
		{
			std::vector<vk::CommandBuffer> v = vk_device.allocateCommandBuffers({
				.commandPool = vk_transfer_command_pool, 
				.level = vk::CommandBufferLevel::ePrimary, 
				.commandBufferCount = 1,
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
			fence = vk_device.createFence({
				.flags = vk::FenceCreateFlagBits::eSignaled,
			});
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
			.flags = vk::ImageCreateFlags{},
			.imageType = vk::ImageType::e2D,
			.format = vk_depth_format, 
			.extent = vk::Extent3D{ctx.client_dimensions.x, ctx.client_dimensions.y, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = vk::SampleCountFlagBits::e1,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
			.sharingMode = vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = vk::ImageLayout::eUndefined,
		});

		// TODO: Read this next time you want to do some memory allocation.
		// Buffers and images should be put in a memory type with device local, not host visible, and with as much memory as possible.
		// Staging buffers should be put in a memory type without device local, with host visible, and with as much memory as possible. If possible, the memory type should not allow host coherent since that is wasting space for uniform buffers that actually need that.
		// Uniform buffers should be put in a memory type with device local and host visible/coherent, and with as little memory as possible. No matter what, it is always guarunteed that there is a memory type with host visible and host coherent, but not necessarily also device local. However, this is fine, as it doesn't really change how the uniform buffer gets used.
		// For all of the above, if possible, a memory type should be chosen without host cached, as it is not really necessary for how I plan to do things. That is, I'm not planning on doing a lot of host memory accesses to GPU memory.
		// Additionally, lazily allocated memory should also be avoided.

		vk::MemoryRequirements vk_depth_image_memory_requirements = vk_device.getImageMemoryRequirements(vk_depth_image);
		vk::DeviceMemory vk_depth_image_memory = vk_device.allocateMemory({
			.allocationSize = vk_depth_image_memory_requirements.size,
			.memoryTypeIndex = vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_depth_image_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			),
		});
		vk_device.bindImageMemory(vk_depth_image, vk_depth_image_memory, 0);
		vk::ImageView vk_depth_image_view = vk_device.createImageView({
			.flags = vk::ImageViewCreateFlags{},
			.image = vk_depth_image,
			.viewType = vk::ImageViewType::e2D,
			.format = vk_depth_format,
			.components = vk::ComponentMapping{},
			.subresourceRange = vk::ImageSubresourceRange{
				.aspectMask = vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
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
		// TODO: Load a gltf file in a way that isn't a complete clusterfuck.

		// I think I've figured out the problem. The problem is that not every index has a valid vertex. Especially many of the later ones don't have one. Clearly, I just don't understand the gltf format. I should really just spend some time reading the spec of gltf so that way I know what I am doing.

		cgltf_data const *box = gltf_load("assets/Box.glb");
		gltf_process_data_result_t res = gltf_process_data(box);
		UNUSED(res);
	#if 0
		cgltf_mesh const &box_mesh = box->meshes[0];
		dprint("{}", box_mesh.name);
		cgltf_material const &box_material = box->materials[0]; // It seems like I can safely ignore the material. The only thing set is the alpha cutoff.
		dprint("{}", box_material.name);
		size_t box_vertex_count = 0;
		for (size_t i = 0; i < box->accessors_count; ++i)
		{
			cgltf_accessor const &accessor = box->accessors[i];
			if (accessor.name) dprint("{}", accessor.name);
			if (accessor.buffer_view->type == cgltf_buffer_view_type_vertices)
			{
				box_vertex_count = accessor.count;
				break;
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
		glm::mat4 box_model = gltf_node_transform_world(&box->nodes[1]);
		cgltf_scene const *box_scene = box->scene;
		if (box_scene->name) dprint("{}", box_scene->name);

		struct vertex_t
		{
			// TODO: Does it matter if we send these as vec4 or vec3?
			// Like, does it just pad it out if we use vec3, making it
			// no different either way? Is it slightly faster than using
			// vec4?
			glm::vec4 pos;
			glm::vec4 normal;
		};
		std::vector<vertex_t> vertices;
		vertices.reserve(box_vertex_count);
		for (size_t i = 0; i < box_vertex_count; ++i)
		{
			vertex_t vertex;
			// TODO: How would this be made generic?
			glm::vec3 normal = *reinterpret_cast<glm::vec3 const *>(reinterpret_cast<uint8_t const *const>(box->bin) + box->accessors[1].offset + i*box->accessors[1].stride);
			glm::vec3 pos = *reinterpret_cast<glm::vec3 const *>(reinterpret_cast<uint8_t const *const>(box->bin) + box->accessors[2].offset + i*box->accessors[2].stride);
			vertex.pos = glm::vec4{pos, 1.0f};
			vertex.normal = glm::vec4{normal, 0.0f};
			glm::mat4 conversion{
				1.0f, 0.0f, 0.0f, 0.0f, 
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f,
			};
			vertex.pos = conversion*vertex.pos;
			vertex.normal = conversion*vertex.normal;
		
			dprint("\n{},{},{} {},{},{}", vertex.pos.x, vertex.pos.y, vertex.pos.z, vertex.normal.x, vertex.normal.y, vertex.normal.z);
			vertices.push_back(vertex);
		}
	#endif

		vk::Buffer vk_vertex_staging_buffer = vk_device.createBuffer({
			.flags = vk::BufferCreateFlags{},
			//.size = sizeof(vertices[0])*vertices.size(),
			//.usage = vk::BufferUsageFlagBits::eTransferSrc,
			//.sharingMode = vk::SharingMode::eExclusive,
			//.queueFamilyIndexCount = 0,
			//.pQueueFamilyIndices = nullptr,
		});
		vk::MemoryRequirements vk_vertex_staging_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_vertex_staging_buffer);
		vk::DeviceMemory vk_vertex_staging_buffer_memory = vk_device.allocateMemory({
			.allocationSize = vk_vertex_staging_buffer_memory_requirements.size,
			.memoryTypeIndex = vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_vertex_staging_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent
			),
		});
		vk_device.bindBufferMemory(vk_vertex_staging_buffer, vk_vertex_staging_buffer_memory, 0);
		//vk_map_memory(vk_device, vk_vertex_staging_buffer_memory, vertices);

		vk::Buffer vk_index_staging_buffer = vk_device.createBuffer({
			.flags = vk::BufferCreateFlags{},
			//.size = box_index_buffer_size,
			//.usage = vk::BufferUsageFlagBits::eTransferSrc,
			//.sharingMode = vk::SharingMode::eExclusive,
			//.queueFamilyIndexCount = 0,
			//.pQueueFamilyIndices = nullptr,
		});
		vk::MemoryRequirements vk_index_staging_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_index_staging_buffer);
		vk::DeviceMemory vk_index_staging_buffer_memory = vk_device.allocateMemory({
			.allocationSize = vk_index_staging_buffer_memory_requirements.size,
			.memoryTypeIndex = vk_find_memory_type_idx(
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
			.flags = vk::BufferCreateFlags{},
			.size = sizeof(uniforms_t),
			.usage = vk::BufferUsageFlagBits::eUniformBuffer,
			//.sharingMode = vk::SharingMode::eExclusive,
			//.queueFamilyIndexCount = 0,
			//.pQueueFamilyIndices = nullptr,
		});
		vk::MemoryRequirements vk_uniform_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_uniform_buffer);
		vk::DeviceMemory vk_uniform_buffer_memory = vk_device.allocateMemory({
			.allocationSize = vk_uniform_buffer_memory_requirements.size,
			.memoryTypeIndex = vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_uniform_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal|
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent
			),
		});
		vk_device.bindBufferMemory(vk_uniform_buffer, vk_uniform_buffer_memory, 0);

		uniforms_t uniforms;
		uniforms.model = glm::mat4{1.0f};
		uniforms.view = glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, 3.0f});
		uniforms.proj = perspective(ctx.aspect_ratio);
		vk_map_memory(vk_device, vk_uniform_buffer_memory, &uniforms, sizeof(uniforms));

		vk::Buffer vk_vertex_buffer = vk_device.createBuffer({
			.flags = vk::BufferCreateFlags{},
			//.size = box_vertex_buffer_size,
			//.usage = vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
			//.sharingMode = vk::SharingMode::eExclusive,
			//.queueFamilyIndexCount = 0,
			//.pQueueFamilyIndices = nullptr,
		});
		vk::MemoryRequirements vk_vertex_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_vertex_buffer);
		vk::DeviceMemory vk_vertex_buffer_memory = vk_device.allocateMemory({
			.allocationSize = vk_vertex_buffer_memory_requirements.size,
			.memoryTypeIndex = vk_find_memory_type_idx(
				vk_physical_device_memory_properties,
				vk_vertex_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			),
		});
		vk_device.bindBufferMemory(vk_vertex_buffer, vk_vertex_buffer_memory, 0);

		vk::Buffer vk_index_buffer = vk_device.createBuffer({
			.flags = vk::BufferCreateFlags{},
			//.size = box_index_buffer_size,
			//.usage = vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eIndexBuffer,
			//.sharingMode = vk::SharingMode::eExclusive,
			//.queueFamilyIndexCount = 0,
			//.pQueueFamilyIndices = nullptr,
		});
		vk::MemoryRequirements vk_index_buffer_memory_requirements = vk_device.getBufferMemoryRequirements(vk_index_buffer);
		vk::DeviceMemory vk_index_buffer_memory = vk_device.allocateMemory({
			.allocationSize = vk_index_buffer_memory_requirements.size,
			.memoryTypeIndex = vk_find_memory_type_idx(
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
	    		sizeof(uniforms_t),
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
				.binding = 0,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eVertex,
				.pImmutableSamplers = nullptr,
			},
		};

		std::array<vk::DescriptorSetLayout, 1>vk_descriptor_set_layouts{};
	    vk_descriptor_set_layouts[0] = vk_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
	    	.flags = vk::DescriptorSetLayoutCreateFlags{},
	    	.bindingCount = static_cast<uint32_t>(vk_descriptor_set_layout_binding_uniform_buffer.size()),
	    	.pBindings = vk_descriptor_set_layout_binding_uniform_buffer.data(),
	    });

	    std::array<vk::DescriptorPoolSize, 1> vk_descriptor_pool_sizes{
	    	vk::DescriptorPoolSize{
	    		.type = vk::DescriptorType::eUniformBuffer,
	    		.descriptorCount = 1,
	    	},
	    };

	    vk::DescriptorPool vk_descriptor_pool = vk_device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
	    	.flags = vk::DescriptorPoolCreateFlags{},
	    	.maxSets = static_cast<uint32_t>(vk_descriptor_pool_sizes.size()),
	    	.poolSizeCount = static_cast<uint32_t>(vk_descriptor_pool_sizes.size()),
	    	.pPoolSizes = vk_descriptor_pool_sizes.data(),
	    });

	    std::vector<vk::DescriptorSet> vk_descriptor_sets = vk_device.allocateDescriptorSets({
	    	.descriptorPool = vk_descriptor_pool,
	    	.descriptorSetCount = static_cast<uint32_t>(vk_descriptor_set_layouts.size()),
	    	.pSetLayouts = vk_descriptor_set_layouts.data(),
	    });

	    std::array<vk::DescriptorBufferInfo, 1> vk_descriptor_buffer_infos{
	    	vk::DescriptorBufferInfo{
	    		.buffer = vk_uniform_buffer,
	    		.offset = 0,
	    		.range = sizeof(uniforms_t),
	    	},
	    };

	    std::array<vk::WriteDescriptorSet, 1> vk_descriptor_writes{
	    	vk::WriteDescriptorSet{
	    		.dstSet = vk_descriptor_sets[0],
	    		.dstBinding = 0, 
	    		.dstArrayElement = 0,
	    		.descriptorCount = static_cast<uint32_t>(vk_descriptor_buffer_infos.size()),
	    		.descriptorType = vk::DescriptorType::eUniformBuffer,
	    		.pImageInfo = nullptr,
	    		.pBufferInfo = vk_descriptor_buffer_infos.data(),
	    		.pTexelBufferView = nullptr,
	    	},
	    };

	    vk_device.updateDescriptorSets(vk_descriptor_writes, {});

	    vk::PipelineLayout vk_pipeline_layout = vk_device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
	    	.flags = vk::PipelineLayoutCreateFlags{},
	    	.setLayoutCount = static_cast<uint32_t>(vk_descriptor_set_layouts.size()),
	    	.pSetLayouts = vk_descriptor_set_layouts.data(),
	    	.pushConstantRangeCount = 0,
	    	.pPushConstantRanges = nullptr,
	    });

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
			{
				.flags = vk_pipeline_cache_flag_bits
				.initialDataSize = 0,
				.pInitialData = nullptr,
			}
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
			.flags = vk::ShaderModuleCreateFlags{},
			.codeSize = static_cast<uint32_t>(slang_spirv_code_vs->getBufferSize()),
			.pCode = static_cast<uint32_t const *>(slang_spirv_code_vs->getBufferPointer()),
		});
		vk::PipelineShaderStageCreateInfo vk_vertex_shader_stage_create_info{
			.flags = vk::PipelineShaderStageCreateFlags{},
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = vk_vertex_shader_module,
			.pName = "main",
			.pSpecializationInfo = nullptr,
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
			.flags = vk::ShaderModuleCreateFlags{},
			.codeSize = static_cast<uint32_t>(slang_spirv_code_ps->getBufferSize()),
			.pCode = static_cast<uint32_t const *>(slang_spirv_code_ps->getBufferPointer()),
		});
		vk::PipelineShaderStageCreateInfo vk_fragment_shader_stage_create_info{
			.flags = vk::PipelineShaderStageCreateFlags{},
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = vk_fragment_shader_module,
			.pName = "main",
			.pSpecializationInfo = nullptr,
		};

		std::array<vk::PipelineShaderStageCreateInfo, 2> vk_shader_stage_create_infos{
			vk_vertex_shader_stage_create_info,
			vk_fragment_shader_stage_create_info,
		};

		std::array<vk::VertexInputBindingDescription, 1> vk_vertex_input_binding_descriptions{
			vk::VertexInputBindingDescription{
				.binding = 0,
				//.stride = sizeof(vertex_t),
				//.inputRate = vk::VertexInputRate::eVertex,
			}
		};

		std::array<vk::VertexInputAttributeDescription, 2> vk_vertex_input_attribute_descriptions{
			vk::VertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = vk::Format::eR32G32B32A32Sfloat,
				//.offset = offsetof(vertex_t, pos),
			},
			vk::VertexInputAttributeDescription{
				.location = 1,
				.binding = 0,
				.format = vk::Format::eR32G32B32A32Sfloat,
				//.offset = offsetof(vertex_t, normal),
			},
		};

		vk::PipelineVertexInputStateCreateInfo vk_vertex_input_state_create_info{
			.flags = vk::PipelineVertexInputStateCreateFlags{},
			.vertexBindingDescriptionCount = static_cast<uint32_t>(vk_vertex_input_binding_descriptions.size()),
			.pVertexBindingDescriptions =vk_vertex_input_binding_descriptions.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_vertex_input_attribute_descriptions.size()),
			.pVertexAttributeDescriptions =vk_vertex_input_attribute_descriptions.data(),
		};

		vk::PipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{
			.flags = vk::PipelineInputAssemblyStateCreateFlags{},
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False,
		};

		std::array<vk::Viewport, 1> vk_viewports{
			vk::Viewport{
				.x = 0.0f,
				.y = 0.0f,
				.width = static_cast<float>(vk_swapchain_extent.width),
				.height = static_cast<float>(vk_swapchain_extent.height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f,
			},
		};

		std::array<vk::Rect2D, 1> vk_scissors{
			vk::Rect2D{
				.offset = vk::Offset2D{0, 0},
				.extent = vk_swapchain_extent,
			},
		};

		vk::PipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info{
			.flags = vk::PipelineViewportStateCreateFlags{},
			.viewportCount = static_cast<uint32_t>(vk_viewports.size()),
			.pViewports = vk_viewports.data(),
			.scissorCount = static_cast<uint32_t>(vk_scissors.size()),
			.pScissors = vk_scissors.data(),

		};

		vk::PipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{
			.flags = vk::PipelineRasterizationStateCreateFlags{},
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eCounterClockwise,
			.depthBiasEnable = vk::False,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};
		vk::PipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info{};

		vk::PipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info{
			.flags = vk::PipelineDepthStencilStateCreateFlags{},
			.depthTestEnable = vk::True,
			.depthWriteEnable = vk::True,
			.depthCompareOp = vk::CompareOp::eGreater, // Because we are using a projection matrix which reverses the depth/z-axis.
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable = vk::False,
			.front = vk::StencilOp{},
			.back = vk::StencilOp{},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 0.0f,
		};

		std::array<vk::PipelineColorBlendAttachmentState, 1> vk_pipeline_color_blend_attachment_states{
			vk::PipelineColorBlendAttachmentState{
				.blendEnable = vk::False,
				.srcColorBlendFactor = vk::BlendFactor::eZero,
				.dstColorBlendFactor = vk::BlendFactor::eZero,
				.colorBlendOp = vk::BlendOp::eAdd,
				.srcAlphaBlendFactor = vk::BlendFactor::eZero,
				.dstAlphaBlendFactor = vk::BlendFactor::eZero,
				.alphaBlendOp = vk::BlendOp::eAdd,
				.colorWriteMask = 
				vk::ColorComponentFlagBits::eR|
				vk::ColorComponentFlagBits::eG|
				vk::ColorComponentFlagBits::eB|
				vk::ColorComponentFlagBits::eA,
			},
		};

		vk::PipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{
			.flags = vk::PipelineColorBlendStateCreateFlags{},
			.logicOpEnable = vk::False,
			.logicOp = vk::LogicOp::eClear,
			.attachmentCount = static_cast<uint32_t>(vk_pipeline_color_blend_attachment_states.size()),
			.pAttachments = vk_pipeline_color_blend_attachment_states.data(),
			.blendConstants = {},
		};

		vk::PipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info{};

		std::array<vk::Format const, 1> const vk_color_attachment_formats{
			vk_swapchain_format,
		};

		vk::PipelineRenderingCreateInfo vk_pipeline_rendering_create_info{
			.viewMask = 0,
			.colorAttachmentCount = static_cast<uint32_t>(vk_color_attachment_formats.size()),
			.pColorAttachmentFormats = vk_color_attachment_formats.data(),
			.depthAttachmentFormat = vk_depth_format,
			//.stencilAttachmentFormat = vk_stencil_format,
		};

		vk::GraphicsPipelineCreateInfo vk_graphics_pipeline_create_info{
			.pNext = &vk_pipeline_rendering_create_info,
	#if BASED_RENDERER_VK_DISABLE_PIPELINE_OPTIMIZATION
			.flags = vk::PipelineCreateFlagBits::eDisableOptimization,
	#else
			.flags = {},
	#endif
			.stageCount = static_cast<uint32_t>(vk_shader_stage_create_infos.size())
			.pStages = vk_shader_stage_create_infos.data(),
			.pVertexInputState = &vk_vertex_input_state_create_info,
			.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info,
			.pTessellationState = nullptr,
			.pViewportState = &vk_pipeline_viewport_state_create_info,
			.pRasterizationState = &vk_pipeline_rasterization_state_create_info,
			.pMultisampleState = &vk_pipeline_multisample_state_create_info,
			.pDepthStencilState = &vk_pipeline_depth_stencil_state_create_info,
			.pColorBlendState = &vk_pipeline_color_blend_state_create_info,
			.pDynamicState = &vk_pipeline_dynamic_state_create_info,
			.layout = vk_pipeline_layout,
			.renderPass = nullptr,
			.subpass = 0,
			.basePipelineHandle = nullptr,
			.basePipelineIndex = 0,
		};

		auto vk_pipelines = *vk_device.createGraphicsPipelines(
			vk_pipeline_cache,
			{
				vk_graphics_pipeline_create_info
			}
		);

		size_t vk_frame_idx = 0;

		ctx.running = true;
		while (ctx.running) 
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

			// TODO: Does this actually have to happen after waitForFences?
			uint32_t vk_image_idx = *vk_device.acquireNextImageKHR(
				vk_swapchain, 
				std::numeric_limits<uint64_t>::max(), 
				vk_semaphores_wait[vk_frame_idx]
			);

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


			update_cube(ctx, vk_device, vk_uniform_buffer_memory, uniforms);

			vk::CommandBuffer cb = vk_graphics_command_buffers[vk_frame_idx];
			cb.begin({
				.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
				.pInheritanceInfo = nullptr,
			});

			// It might seem unnecessarily verbose that I am doing things this way. Why not just create the array once, and then modify it based on the value of "staged"? In my experience, I have found that when I am not this explicit about things in Vulkan, it makes it a lot harder to find mistakes.
			static size_t staged = 0;
			if (staged == 0)
			{
				std::array<vk::BufferMemoryBarrier2, 4> buffer_barriers{
					vk::BufferMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlags2{},
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
						.dstAccessMask = vk::AccessFlagBits2::eTransferRead,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.buffer = vk_vertex_staging_buffer,
						.offset = 0,
						//.size = box_vertex_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlags2{},
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
						.dstAccessMask = vk::AccessFlagBits2::eTransferRead,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.buffer = vk_index_staging_buffer,
						.offset = 0,
						//.size = box_index_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlags2{},
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
						.dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.buffer = vk_vertex_buffer,
						.offset = 0,
						//.size = box_vertex_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlags2{},
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
						.dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.buffer = vk_index_buffer,
						.offset = 0,
						//.size = box_index_buffer_size,
					},
				};

				std::array<vk::ImageMemoryBarrier2, 2> image_barriers{
					vk::ImageMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eColorAttachmentOutput},
						.dstAccessMask = vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
						.oldLayout = vk::ImageLayout::eUndefined,
						.newLayout = vk::ImageLayout::eColorAttachmentOptimal,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.image = vk_swapchain_images[vk_image_idx],
						.subresourceRange = vk::ImageSubresourceRange{
							.aspectMask = vk::ImageAspectFlagBits::eColor,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
						},
					},
					vk::ImageMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests|vk::PipelineStageFlagBits2::eLateFragmentTests,
						.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
						.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests|vk::PipelineStageFlagBits2::eLateFragmentTests,
						.dstAccessMask = vk::AccessFlags2{vk::AccessFlagBits2::eColorAttachmentWrite},
						.oldLayout = vk::ImageLayout::eUndefined,
						.newLayout = vk::ImageLayout::eDepthAttachmentOptimal,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.image = vk_depth_image,
						.subresourceRange = vk::ImageSubresourceRange{
							.aspectMask = vk::ImageAspectFlagBits::eDepth,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
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
						0,
						0,
						vk_image,
						vk::ImageSubresourceRange{
							.aspectMask = vk::ImageAspectFlagBits::eColor,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
						},
					},
	#endif
				};

				cb.pipelineBarrier2({
					.dependencyFlags = vk::DependencyFlags{},
					.memoryBarrierCount = 0,
					.pMemoryBarriers = nullptr,
					.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size()),
					.pBufferMemoryBarriers = buffer_barriers.data(),
					.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
					.pImageMemoryBarriers = image_barriers.data(),
				});

				std::array<vk::BufferCopy, 1> vertex_buffer_regions{
					vk::BufferCopy{
						.srcOffset = 0,
						.dstOffset = 0,
						//.size = box_vertex_buffer_size,
					},
				};

				cb.copyBuffer(vk_vertex_staging_buffer, vk_vertex_buffer, vertex_buffer_regions);

				std::array<vk::BufferCopy, 1> index_buffer_regions{
					vk::BufferCopy{
						.srcOffset = 0,
						.dstOffset = 0,
						//.size = box_index_buffer_size,
					},
				};

				cb.copyBuffer(vk_index_staging_buffer, vk_index_buffer, index_buffer_regions);

				std::array<vk::BufferMemoryBarrier2, 2> buffer_barriers2{
					vk::BufferMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
						.srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
						.dstStageMask = vk::PipelineStageFlagBits2::eVertexAttributeInput,
						.dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.buffer = vk_vertex_buffer,
						.offset = 0,
						//.size = box_vertex_buffer_size,
					},
					vk::BufferMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
						.srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
						.dstStageMask = vk::PipelineStageFlagBits2::eIndexInput,
						.dstAccessMask = vk::AccessFlagBits2::eIndexRead
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.buffer = vk_index_buffer,
						.offset = 0,
						//.size = box_index_buffer_size,
					},
				};

				cb.pipelineBarrier2({
					.dependencyFlags = vk::DependencyFlags{},
					.memoryBarrierCount = 0,
					.pMemoryBarriers = nullptr,
					.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers2.size()),
					.pBufferMemoryBarriers = buffer_barriers2.data(),
					.imageMemoryBarrierCount = 0,
					.pImageMemoryBarriers = nullptr,
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
						.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
						.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
						.oldLayout = vk::ImageLayout::eUndefined,
						.newLayout = vk::ImageLayout::eColorAttachmentOptimal,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.image = vk_swapchain_images[vk_image_idx],
						.subresourceRange = vk::ImageSubresourceRange{
							.aspectMask = vk::ImageAspectFlagBits::eColor,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
						},
					},
				};

				cb.pipelineBarrier2({
					.dependencyFlags = vk::DependencyFlags{},
					.memoryBarrierCount = 0,
					.pMemoryBarriers = nullptr,
					.bufferMemoryBarrierCount = 0,
					.pBufferMemoryBarriers = nullptr,
					.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
					.pImageMemoryBarriers = image_barriers.data(),
				});

				staged += 1;
			}
			else
			{
				std::array<vk::ImageMemoryBarrier2, 1> image_barriers{
					vk::ImageMemoryBarrier2{
						.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
						.srcAccessMask = vk::AccessFlags2{},
						.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
						.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
						.oldLayout = vk::ImageLayout::ePresentSrcKHR,
						.newLayout = vk::ImageLayout::eColorAttachmentOptimal,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.image = vk_swapchain_images[vk_image_idx],
						.subresourceRange = vk::ImageSubresourceRange{
							.aspectMask = vk::ImageAspectFlagBits::eColor,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
						},
					},
				};

				cb.pipelineBarrier2({
					.dependencyFlags = vk::DependencyFlags{},
					.memoryBarrierCount = 0,
					.pMemoryBarriers = nullptr,
					.bufferMemoryBarrierCount = 0,
					.pBufferMemoryBarriers = nullptr,
					.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
					.pImageMemoryBarriers = image_barriers.data(),
				});
			}

			std::array<vk::RenderingAttachmentInfo, 1> vk_rendering_attachment_infos{
				vk::RenderingAttachmentInfo{
					.imageView = vk_swapchain_image_views[vk_image_idx],
					.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,

					.resolveMode = vk::ResolveModeFlagBits::eNone,
					.resolveImageView = vk::ImageView{},
					.resolveImageLayout = vk::ImageLayout::eUndefined,

					.loadOp = vk::AttachmentLoadOp::eClear,
					.storeOp = vk::AttachmentStoreOp::eStore,
				},
			};
			// TODO: Figure out a way to do this inside the designated initializer.
			vk_rendering_attachment_infos[0].clearValue.color.float32[0] = 0.0f;
			vk_rendering_attachment_infos[0].clearValue.color.float32[1] = 0.0f;
			vk_rendering_attachment_infos[0].clearValue.color.float32[2] = 0.0f;
			vk_rendering_attachment_infos[0].clearValue.color.float32[3] = 1.0f;

			vk::RenderingAttachmentInfo vk_depth_attachment_info{
				.imageView = vk_depth_image_view,
				.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,

				.resolveMode = vk::ResolveModeFlagBits::eNone,
				.resolveImageView = vk::ImageView{},
				.resolveImageLayout = vk::ImageLayout::eUndefined,

				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eDontCare,
			};
			// TODO: Figure out a way to do this inside the designated initializer.
			vk_depth_attachment_info.clearValue.depthStencil.depth = 0.0f;
			vk_depth_attachment_info.clearValue.depthStencil.stencil = 0;

			cb.beginRendering({
				.flags = vk::RenderingFlags{},
				.renderArea = vk::Rect2D{
					.offset = vk::Offset2D{0, 0},
					.extent = vk_swapchain_extent,
				},
				.layerCount = 1,
				.viewMask = 0,
				.colorAttachmentCount = static_cast<uint32_t>(vk_rendering_attachment_infos.size()),
				.pColorAttachments = vk_rendering_attachment_infos.data(),
				.pDepthAttachment = &vk_depth_attachment_info,
				//.pStencilAttachment = &vk_stencil_attachment_info,
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
						.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
						.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
						.dstStageMask = vk::PipelineStageFlags2{},
						.dstAccessMask = vk::AccessFlags2{},
						.oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
						.newLayout = vk::ImageLayout::ePresentSrcKHR,
						.srcQueueFamilyIndex = 0,
						.dstQueueFamilyIndex = 0,
						.image = vk_swapchain_images[vk_image_idx],
						.subresourceRange = vk::ImageSubresourceRange{
							.aspectMask = vk::ImageAspectFlagBits::eColor,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
						},
					},
				};

				cb.pipelineBarrier2({
					.dependencyFlags = vk::DependencyFlags{},
					.memoryBarrierCount = 0,
					.pMemoryBarriers = nullptr,
					.bufferMemoryBarrierCount = 0,
					.pBufferMemoryBarriers = nullptr,
					.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
					.pImageMemoryBarriers = image_barriers.data(),
				});
			}

			cb.end();

			std::array<vk::SemaphoreSubmitInfo, 1> vk_wait_semaphore_infos{
				vk::SemaphoreSubmitInfo{
					.semaphore = vk_semaphores_wait[vk_frame_idx],
					.value = 0,
					.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
					.deviceIndex = 0,
				},
			};

			std::array<vk::CommandBufferSubmitInfo, 1> vk_command_buffer_submit_infos{
				vk::CommandBufferSubmitInfo{
					.commandBuffer = cb,
					.deviceMask = 0,
				},
			};

			std::array<vk::SemaphoreSubmitInfo, 1> vk_signal_semaphore_infos{
				vk::SemaphoreSubmitInfo{
					.semaphore = vk_semaphores_signal[vk_frame_idx],
					.value = 0,
					.stageMask = vk::PipelineStageFlagBits2::eAllCommands,
					.deviceIndex = 0,
				},
			};

			std::array<vk::SubmitInfo2, 1> vk_submit_infos{
				vk::SubmitInfo2{
					.flags = vk::SubmitFlags{},
					.waitSemaphoreInfoCount = static_cast<uint32_t>(vk_wait_semaphore_infos.size()),
					.pWaitSemaphoreInfos = vk_wait_semaphore_infos.data(),
					.commandBufferInfoCount = static_cast<uint32_t>(vk_command_buffer_submit_infos.size()),
					.pCommandBufferInfos = vk_command_buffer_submit_infos.data(),
					.signalSemaphoreInfoCount = static_cast<uint32_t>(vk_signal_semaphore_infos.size()),
					.pSignalSemaphoreInfos = vk_signal_semaphore_infos.data(),
				}
			};
			vk_graphics_queue.submit2(vk_submit_infos, vk_fences[vk_frame_idx]);

			std::array<vk::Semaphore, 1> vk_present_wait_semaphores{vk_semaphores_signal[vk_frame_idx]};
			std::array<vk::SwapchainKHR, 1> vk_present_swapchains{vk_swapchain};
			std::array<uint32_t, 1> vk_present_image_indices{vk_image_idx};
			std::array<vk::Result, 1> vk_present_results;
			// TODO: Use the present queue.
			vk::detail::resultCheck(vk_graphics_queue.presentKHR({
				.waitSemaphoreCount = static_cast<uint32_t>(vk_present_wait_semaphores.size()),
				.pWaitSemaphores = vk_present_wait_semaphores.data(),
				.swapchainCount = static_cast<uint32_t>(vk_present_swapchains.size()),
				.pSwapchains = vk_present_swapchains.data(),
				.pImageIndices = vk_present_image_indices.data(),
				.pResults = vk_present_results.data(),
			}), "Failed to present.");
			vk::detail::resultCheck(vk_present_results[0], "Failed to present.");

			vk_frame_idx = (vk_frame_idx + 1) % vk_swapchain_images.size();
		}
	}

	static std::vector<char const *> vk_get_instance_extensions()
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

	static std::vector<char const *> vk_get_device_extensions(vk::PhysicalDevice const physical_device)
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

	static std::vector<vk::DeviceQueueCreateInfo> vk_get_device_queue_infos(std::vector<vk::QueueFamilyProperties> const &queue_family_properties)
	{
		std::vector<vk::DeviceQueueCreateInfo> device_queue_infos;
		device_queue_infos.reserve(queue_family_properties.size());

		for (size_t i = 0; i < queue_family_properties.size(); ++i)
		{
			if (queue_family_properties[i].queueCount > 0)
			{
				device_queue_infos.push_back(vk::DeviceQueueCreateInfo{
					.flags = vk::DeviceQueueCreateFlags{},
					.queueFamilyIndex = static_cast<uint32_t>(i),
					.queueCount = queue_family_properties[i].queueCount,
					.pQueuePriorities = vk_queue_priorities.data(),
				});
			}
		}

		return device_queue_infos;
	}

	static std::vector<std::vector<vk::Queue>> vk_get_queues(vk::Device const device, std::vector<vk::QueueFamilyProperties> const &queue_family_properties)
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

	static size_t vk_find_queue_family_idx(std::vector<vk::QueueFamilyProperties> const &queue_family_properties, vk::QueueFlagBits const flags)
	{
		for (size_t i = 0; i < queue_family_properties.size(); ++i)
		{
			if ((queue_family_properties[i].queueFlags & vk::QueueFlags{flags}) != vk::QueueFlags{})
			{
				return i;
			}
		}
		throw vk::LogicError{FORMAT_ERROR("Failed to find queue family idx")};
	}

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

	static uint32_t vk_find_memory_type_idx(
		vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
		uint32_t const memory_type_bits,
		vk::MemoryPropertyFlags const required_memory_properties)
	{
		for (
			uint32_t memory_type_idx = 0; 
			memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
			++memory_type_idx)
		{
			uint32_t memory_type_bit = 1 << memory_type_idx;		
			vk::MemoryPropertyFlags memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
			if ((memory_type_bits&memory_type_bit) && ((required_memory_properties&memory_properties) == required_memory_properties))
			{
				return memory_type_idx;
			}
		}

		throw vk::LogicError{FORMAT_ERROR("Failed to find memory type index with the required memory properties!")};
	}

	static uint32_t vk_find_memory_type_idx(
		vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
		uint32_t const memory_type_bits,
		vk::MemoryPropertyFlags const required_memory_properties,
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

		for (
			uint32_t memory_type_idx = 0; 
			memory_type_idx < physical_device_memory_properties.memoryTypeCount; 
			++memory_type_idx)
		{
			uint32_t memory_type_bit = 1 << memory_type_idx;		
			vk::MemoryPropertyFlags memory_properties = physical_device_memory_properties.memoryTypes[memory_type_idx].propertyFlags;
			if ((memory_type_bits&memory_type_bit) && ((required_memory_properties&memory_properties) == required_memory_properties))
			{
				return memory_type_idx;
			}
		}

		throw vk::LogicError{FORMAT_ERROR("Failed to find memory type index with the required memory properties!")};
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