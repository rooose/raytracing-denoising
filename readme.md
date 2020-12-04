# Raytracing Denoising

This project is done for the course INF8702 at Polytechnique of Montr�al.

## Getting started

In order to make this repository, Vulkan is required (https://vulkan.lunarg.com/) as well as the Vulkan-beta driver for
raytracing (https://developer.nvidia.com/vulkan-driver). You'll also need cmake and Visual Studio 2019 to build.

Get GLFW, GLI, GLM, STB, TinyGLTF, TinyOBJLoader and VK_GLTF if you don't have them by getting the submodules

`git submodule update --init --recursive`

To build, run the command:

`cmake -B out/build`

You can open the generated solution in Visual Studio 2019. If you get the Access Denied error on ALL_BUILD, right click
on the project and set it as the principal project.

## Authors

Adem Aber Aouni @ThePhosphorus
Rose Hirigoyen @rooose