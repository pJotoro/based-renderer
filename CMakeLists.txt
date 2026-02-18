cmake_minimum_required(VERSION 4.2)

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Shared: ${TARGET_SUPPORTS_SHARED_LIBS}")

find_package(Vulkan REQUIRED)

project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_compile_features(based_renderer PUBLIC cxx_std_20)
target_link_libraries(based_renderer PUBLIC Vulkan::Vulkan)
target_precompile_headers(based_renderer PUBLIC src/pch.hpp)

if(MSVC)
target_compile_options(based_renderer PUBLIC /W4 /WX /diagnostics:column)
target_link_options(based_renderer PUBLIC /subsystem:windows)
endif()