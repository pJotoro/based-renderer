# TODO: Once the project is done, lower the minimum required version to the lowest possible number that doesn't cause anything to break.
cmake_minimum_required(VERSION 4.2)

# HACK: For some reason, CMake thinks Windows doesn't support shared libraries. I have no idea why this is.
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

find_package(Vulkan 1.4.0 REQUIRED)

set(VULKAN_SDK "$ENV{VULKAN_SDK}")
add_library(slang SHARED IMPORTED)
set_target_properties(slang PROPERTIES
    IMPORTED_LOCATION "${VULKAN_SDK}/Lib/slang.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_SDK}/Include"
)
add_library(slang_compiler SHARED IMPORTED)
set_target_properties(slang_compiler PROPERTIES
    IMPORTED_LOCATION "${VULKAN_SDK}/Lib/slang-compiler.lib"
)

project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_compile_features(based_renderer PRIVATE cxx_std_20)
target_precompile_headers(based_renderer PRIVATE src/pch.hpp)
target_link_libraries(slang INTERFACE slang_compiler)
target_link_libraries(based_renderer PRIVATE Vulkan::Vulkan slang)

# TODO: Is this the only way to set these options? Or is there a cross-platform way?
# TODO: What about other compilers? I know you can also use clang and mingw on Windows.
if(MSVC)
target_compile_options(based_renderer PRIVATE /W4 /WX /diagnostics:column)
target_link_options(based_renderer PRIVATE /subsystem:windows)
endif()