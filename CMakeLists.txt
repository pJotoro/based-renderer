cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

project(based_renderer CXX)
add_executable(based_renderer src/main.cpp)
target_link_libraries(based_renderer PRIVATE glm::glm Vulkan::Vulkan)