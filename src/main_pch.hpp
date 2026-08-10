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

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#error TODO: Implement other platforms.
#endif
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_SETTERS
#define VULKAN_HPP_HAS_UNRESTRICTED_UNIONS
#include <vulkan/vulkan.hpp>

#include <slang/slang-com-ptr.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/rotate_normalized_axis.hpp>

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