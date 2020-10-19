cmake_minimum_required( VERSION 3.15)

project(raytracing-denoising)

# Defines
set(CMAKE_CXX_STANDARD 20)
set(SOURCES "src/main.cpp")

# Include third-parties
add_subdirectory("${CMAKE_SOURCE_DIR}/third-party")
find_package(Vulkan REQUIRED FATAL_ERROR)


add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})




target_link_libraries(${PROJECT_NAME} glm::glm glfw Vulkan::Vulkan)
