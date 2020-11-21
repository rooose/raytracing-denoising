cmake_minimum_required( VERSION 3.15)

project(raytracing-denoising)

# Defines
set(CMAKE_CXX_STANDARD 20)
file (GLOB_RECURSE SHADERS
    RELATIVE "${SOURCE_DIR}/src"
    "${SOURCE_DIR}src/shaders/*.vert"
    "${SOURCE_DIR}src/shaders/*.frag"
    "${SOURCE_DIR}src/shaders/*.rgen"
    "${SOURCE_DIR}src/shaders/*.rmiss"
    "${SOURCE_DIR}src/shaders/*.rchit")
file(GLOB SOURCES
    "${SOURCE_DIR}src/*.cpp")
file(GLOB HEADERS
    "${SOURCE_DIR}src/*.hpp"
    "${SOURCE_DIR}src/*.h")
source_group(shaders FILES ${SHADERS})

# Include third-parties
add_subdirectory("${CMAKE_SOURCE_DIR}/third-party")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/shaders")
find_package(Vulkan REQUIRED FATAL_ERROR)

include_directories("${CMAKE_SOURCE_DIR}/third-party/stb")
include_directories("${CMAKE_SOURCE_DIR}/third-party/tinygltf")
include_directories("${CMAKE_SOURCE_DIR}/third-party/vkgLTF/base")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SHADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} glm::glm glfw Vulkan::Vulkan tinyobjloader gli)

add_dependencies(${PROJECT_NAME} Shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        )