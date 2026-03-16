# TODO: Once the project is done, lower the minimum required version to the lowest possible number that doesn't cause anything to break.
cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

project(based_renderer CXX)

# I got a lot of help from this:
# https://github.com/KhronosGroup/Vulkan-Tutorial/blob/main/attachments/CMakeLists.txt

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

set(CMAKE_CXX_MODULE_STD "1" ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
add_compile_definitions(VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING)

find_package(Vulkan 1.4 REQUIRED)

add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule
      PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)
target_include_directories(VulkanCppModule
      PUBLIC
      "${Vulkan_INCLUDE_DIR}"
)
target_link_libraries(VulkanCppModule
      PUBLIC
      Vulkan::Vulkan
)

set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 23)

# Add MSVC-specific compiler options for proper C++ module support
if(MSVC)
target_compile_options(VulkanCppModule PRIVATE
  /std:c++latest      # Use latest C++ standard for better module support
  /permissive-        # Standards conformance mode
  /Zc:__cplusplus     # Enable correct __cplusplus macro
  /EHsc               # Enable C++ exception handling
  /Zc:preprocessor    # Use conforming preprocessor
  /translateInclude   # Automatically translate #include to import for standard library
)
endif()

target_sources(VulkanCppModule
      PUBLIC
      FILE_SET cxx_modules TYPE CXX_MODULES
      BASE_DIRS
      "${Vulkan_INCLUDE_DIR}"
      FILES
      "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

# Add the vulkan.cppm file directly as a source file
target_sources(VulkanCppModule
      PRIVATE
      "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

# TODO: How to find Slang?

add_executable(based_renderer src/main.cpp)
target_compile_features(based_renderer PRIVATE cxx_std_23)
target_link_libraries(based_renderer PRIVATE Vulkan::cppm slang)

# TODO: Is this the only way to set these options? Or is there a cross-platform way?
# TODO: What about other compilers? I know you can also use clang and mingw on Windows.
if(MSVC)
target_compile_options(based_renderer PRIVATE /W4 /WX /diagnostics:column)
target_link_options(based_renderer PRIVATE /subsystem:windows)
endif()