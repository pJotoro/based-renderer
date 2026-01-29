cmake_minimum_required(VERSION 4.2)
project(based_renderer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

add_executable(based_renderer src/main.cpp)
target_compile_options(based_renderer PRIVATE /W4 /WX /wd4456 /wd4552 /wd4553 /wd4127 /diagnostics:column /EHs- /EHc-)
target_link_libraries(based_renderer PRIVATE Vulkan::Vulkan glm::glm SDL3::SDL3)