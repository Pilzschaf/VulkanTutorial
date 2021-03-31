cmake_minimum_required(VERSION 3.10)

# Project name
project(vulkan_tutorial)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set(SOURCE_FILES src/main.cpp)

# Find SDL2
add_subdirectory(libs/SDL)

# Find Vulkan
find_package(Vulkan REQUIRED)

# vulkan_tutorial executable
add_executable(vulkan_tutorial ${SOURCE_FILES})
target_include_directories(vulkan_tutorial PUBLIC libs/SDL/include)
target_link_libraries(vulkan_tutorial PUBLIC SDL2-static)
target_include_directories(vulkan_tutorial PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_tutorial PUBLIC ${Vulkan_LIBRARIES})