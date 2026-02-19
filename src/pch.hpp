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
// As far as I can tell, VULKAN_HPP_TYPESAFE_CONVERSION is needed in order to allow assigning certain vulkan handles.
#include <vulkan/vulkan.hpp>

#include <slang/slang.h>

// #include <glm/glm.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>