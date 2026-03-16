# TODO: Once the project is done, lower the minimum required version to the lowest possible number that doesn't cause anything to break.
cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

add_compile_definitions(VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING)

find_package(Vulkan 1.4 REQUIRED)

add_library(std OBJECT src/std.ixx)

project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_compile_features(based_renderer PRIVATE cxx_std_23)
target_link_libraries(based_renderer PRIVATE Vulkan::Vulkan std)
target_sources(based_renderer PRIVATE
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${Vulkan_INCLUDE_DIR}"
    FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

# TODO: Is this the only way to set these options? Or is there a cross-platform way?
# TODO: What about other compilers? I know you can also use clang and mingw on Windows.
if(MSVC)
target_compile_options(based_renderer PRIVATE /W4 /WX /diagnostics:column)
target_link_options(based_renderer PRIVATE /subsystem:windows)
endif()