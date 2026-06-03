#pragma once

#ifdef UNUSED
#warning Removing previously defined macro 'UNUSED'.
#undef UNUSED
#endif
#define UNUSED(X) (void)(X)

#ifdef STRINGIFY
#warning Removing previously defined macro 'STRINGIFY'.
#undef STRINGIFY
#endif
#define STRINGIFY(x) #x

#ifdef STMT
#warning Removing previously defined macro 'STMT'.
#undef STMT
#endif
#define STMT(X) do {X} while (0)

#ifdef FORMAT_ERROR
#warning Removing previously defined macro 'FORMAT_ERROR'.
#undef FORMAT_ERROR
#endif
#define FORMAT_ERROR(MESSAGE) std::format("{}({}): {}", __FUNCTION__, __LINE__, (MESSAGE))

#ifdef _DEBUG
#define BASED_RENDERER_DEBUG 1
#else
#define BASED_RENDERER_DEBUG 0
#endif

#define BASED_RENDERER_VK_DEBUG BASED_RENDERER_DEBUG
#define BASED_RENDERER_VK_LAYERS 0
#define BASED_RENDERER_VK_DEBUG_OUTPUT BASED_RENDERER_VK_DEBUG
#define BASED_RENDERER_VK_DISABLE_PIPELINE_OPTIMIZATION BASED_RENDERER_VK_DEBUG

#define BASED_RENDERER_SLANG_DEBUG BASED_RENDERER_VK_DEBUG
#define BASED_RENDERER_SLANG_SPIRV_VALIDATION BASED_RENDERER_SLANG_DEBUG

#define BASED_RENDERER_FULLSCREEN 1

// TODO: What about other systems?
#define VK_KHR_platform_surface "VK_KHR_win32_surface"