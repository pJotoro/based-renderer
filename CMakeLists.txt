# TODO: Once the project is done, lower the minimum required version to the lowest possible number that doesn't cause anything to break. Also tell CMake to treat all CMake code as being of that version.
cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# ===== Set up libraries =====
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE) # HACK: For some reason, CMake thinks Windows doesn't support shared libraries. I have no idea why this is.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}") # Needed in order for CMake to find .cmake files in the current directory.

find_package(Vulkan 1.4.0 REQUIRED)
set(VULKAN_SDK "$ENV{VULKAN_SDK}")

add_library(slang STATIC IMPORTED)
set_target_properties(slang PROPERTIES
    IMPORTED_LOCATION "${VULKAN_SDK}/Lib/slang.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_SDK}/Include")

find_package(stb REQUIRED)
find_package(cgltf REQUIRED)

# ============================

# ===== Set up project =====
project(based_renderer CXX)
add_executable(based_renderer src/main.cpp src/libraries.cpp)
target_precompile_headers(based_renderer PRIVATE src/main_pch.hpp)
target_link_libraries(based_renderer PRIVATE Vulkan::Vulkan slang stb::stb cgltf::cgltf)

if(MSVC)
target_compile_options(based_renderer PRIVATE /W4 /WX /diagnostics:column /wd4201 /wd4324)
target_link_options(based_renderer PRIVATE /subsystem:windows)
else()
    message(FATAL_ERROR "TODO: Implement other platforms." )
endif()
# ==========================