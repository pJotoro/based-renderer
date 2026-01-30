cmake_minimum_required(VERSION 4.2)
find_package(Vulkan REQUIRED)

project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_compile_features(based_renderer PRIVATE cxx_std_20)
target_link_libraries(based_renderer PRIVATE Vulkan::Vulkan)
target_precompile_headers(based_renderer PRIVATE src/pch.hpp)

if(MSVC)
target_compile_options(based_renderer PRIVATE /W4 /WX /diagnostics:column)
target_link_options(based_renderer PRIVATE /subsystem:windows)
endif()