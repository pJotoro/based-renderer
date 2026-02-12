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

// #include <glm/glm.hpp>

#include <algorithm>
#include <format>
#include <optional>
#include <span>