# TODO: Once the project is done, lower the minimum required version to the lowest possible number that doesn't cause anything to break.
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
    INTERFACE_INCLUDE_DIRECTORIES "$ENV{VULKAN_SDK}/Include")

add_library(slang_compiler STATIC IMPORTED)
set_target_properties(slang_compiler PROPERTIES
    IMPORTED_LOCATION "${VULKAN_SDK}/Lib/slang-compiler.lib")

find_package(glm REQUIRED)

# TODO: What should be done when it comes to single-header-libraries? For now, it seems like it would be much easier to just copy-paste them into the project, but eventually, wouldn't it make sense to use them in CMake just like other libraries?

# ============================

# ===== Set up project =====
project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_precompile_headers(based_renderer PRIVATE src/pch.hpp)
target_link_libraries(slang INTERFACE slang_compiler)
target_link_libraries(based_renderer PRIVATE Vulkan::Vulkan slang glm::glm)

# TODO: Is this the only way to set these options? Or is there a cross-platform way?
# TODO: What about other compilers? I know you can also use clang and mingw on Windows.
if(MSVC)
target_compile_options(based_renderer PRIVATE /W4 /WX /diagnostics:column /wd4201)
target_link_options(based_renderer PRIVATE /subsystem:windows)
endif()
# ==========================