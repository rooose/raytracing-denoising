cmake_minimum_required(VERSION 3.15)

project(third-party)

# Describe all third party libraries

add_subdirectory(glfw)

add_subdirectory(glm)

add_subdirectory(tinyobjloader)

set(GLI_TEST_ENABLE OFF)

add_subdirectory(gli)
