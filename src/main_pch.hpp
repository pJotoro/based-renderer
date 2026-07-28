#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Windows.h defines these macros, which screw with certain things in the C++ standard library.
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#include <slang/slang-com-ptr.h>

#include <glm/ext/vector_float2.hpp> // vec2
#include <glm/ext/vector_float3.hpp> // vec3
#include <glm/ext/vector_float4.hpp> // vec4
#include <glm/ext/matrix_float3x3.hpp> // mat3
#include <glm/ext/matrix_float4x4.hpp> // mat4
#include <glm/ext/vector_int2.hpp> // ivec2
#include <glm/ext/vector_uint2.hpp> // uvec2
#include <glm/ext/matrix_transform.hpp> // translate, rotate
#include <glm/ext/matrix_clip_space.hpp> // perspective

#include <algorithm>
#include <format>
// #include <fstream>
#include <optional>
#include <span>
// #include <sstream>

#include <stb_image.h>

#pragma warning(push)
#pragma warning(disable: 4996)
#include <cgltf.h>
#pragma warning(pop)