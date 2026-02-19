# TODO: Once the project is done, lower the minimum required version to the lowest possible number that doens't cause anything to break.
cmake_minimum_required(VERSION 4.2)

find_package(Vulkan 1.4.0 REQUIRED)

# TODO: Once the project is done, change as many mentions of PUBLIC to PRIVATE as possible.

project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_compile_features(based_renderer PUBLIC cxx_std_20)
target_link_libraries(based_renderer PUBLIC Vulkan::Vulkan) # TODO: Find out how the library/package naming convention works. As in, why is it Vulkan::Vulkan? Why not just Vulkan?
target_precompile_headers(based_renderer PUBLIC src/pch.hpp)

# TODO: Is this the only way to set these options? Or is there a cross-platform way?
# TODO: What about other compilers? I know you can also use clang and mingw on Windows.
if(MSVC)
target_compile_options(based_renderer PUBLIC /W4 /WX /diagnostics:column)
target_link_options(based_renderer PUBLIC /subsystem:windows)
endif()