#include "Application.hpp"
#include "ShaderModule.hpp"

#ifdef _DEBUG
#include "vkValidation.hpp"
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <set>
#include <unordered_map>

#include <stb_image.h>

#include <tiny_gltf.h>

#define GetTypeSizeInBytes(x) GetNumComponentsInType(x)

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <algorithm>

#define FPS_COUNTER_TOP 256

uint8_t Application::_verbose = 0;

void Application::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::setVerbose(uint8_t verboseMode)
{
    _verbose = verboseMode;
}

uint8_t Application::getVerbose()
{
    return _verbose;
}

void Application::setFrameBufferResize()
{
    _framebufferResized = true;
}

Character& Application::getCharacter()
{
    return _character;
}

void Application::drawFrame()
{
    vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { _swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Fialed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Tutorial", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->setFrameBufferResize();
    });

    glfwSetWindowFocusCallback(_window, [](GLFWwindow* window, int focused) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->getCharacter().onFocus(window, focused);
    });
}

void Application::initVulkan()
{
    createVKInstance();
#ifdef _DEBUG
    setupDebugMessenger();
#endif

    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    //createImageViews();
    //createRenderPass();
    createCommandPool();
    createStorageImage();
    //createDepthResources();
    //createFrameBuffers();
    _samplers.emplace_back(*this);
    //_textures.emplace_back(*this, _samplers[0], TEXTURE_PATH);
    _models.emplace_back(*this);
    _models[0].loadModel(MODEL_PATH);
    createDescriptorSetLayout();
    _models[0].load(_indices, _vertices);
    _rtHandler.init();
    createGraphicsPipeline();
    createShaderBindingTable();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSemaphores();
}

void Application::createVKInstance()
{
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Tutorial";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef _DEBUG
    if (!checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

#if _VK_VALID_GOOD_PRACTICES
    VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
    VkValidationFeaturesEXT features = {};
    features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    features.enabledValidationFeatureCount = 1;
    features.pEnabledValidationFeatures = enables;
    createInfo.pNext = &features;
#endif

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else
    createInfo.enabledLayerCount = 0;

#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    VkResult status = vkCreateInstance(&createInfo, nullptr, &_instance);

    if (status != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    // Get all available extensions
    if (_verbose > 1) {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }
}

void Application::mainLoop()
{
    size_t counter = 0;
    float timeSum = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(_window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = std::chrono::high_resolution_clock::now();
        glfwPollEvents();
        _character.update(_window, time);
        drawFrame();

        timeSum += time;
        counter++;

        if (counter >= FPS_COUNTER_TOP) {
            std::cout << "FPS : " << static_cast<float>(counter) / timeSum << std::endl;
            counter = 0;
            timeSum = 0.f;
        }
    }

    vkDeviceWaitIdle(_device);
}

void Application::cleanup()
{
    _samplers.clear();
    _textures.clear();
    _models.clear();

    cleanupSwapchain();

    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayouts.raytrace, nullptr);
    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayouts.textures, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_device, _inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(_device, _commandPool, nullptr);

    vkDestroyDevice(_device, nullptr);

    vkDestroySurfaceKHR(_instance, _surface, nullptr);

#ifdef _DEBUG
    DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
#endif

    vkDestroyInstance(_instance, nullptr);

    glfwDestroyWindow(_window);

    glfwTerminate();
}

void Application::createSurface()
{
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create wiondow surface!");
    }
}

void Application::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    } else if (_verbose > 1) {
        std::cout << "Found " << deviceCount << " GPUs" << std::endl;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    float bestScore = 0;

    for (const auto& device : devices) {
        float score = 0;
        // Determine the available device local memory.
        VkPhysicalDeviceMemoryProperties memoryProps {};
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProps);

        auto heapsPointer = memoryProps.memoryHeaps;
        auto heaps = std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProps.memoryHeapCount);

        for (const auto& heap : heaps) {
            if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                // Device local heap, should be size of total GPU VRAM.
                //heap.size will be the size of VRAM in bytes. (bigger is better)
                score += heap.size;
            }
        }

        VkPhysicalDeviceProperties props {};
        vkGetPhysicalDeviceProperties(device, &props);

        // Determine the type of the physical device
        if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score *= 4; // Prefer Discrete
        } else if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score *= 1;
        } else {
            score *= -1; // Crash Program
        }

        if (!isDeviceSuitable(device)) {
            score = 0;
        }

        if (score > bestScore) {
            bestDevice = device;
            bestScore = score;
        }
    }

    _physDevice = bestDevice;
    if (_physDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Found no suitable GPUs");
    }

    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(_physDevice, &props);

    if (_verbose > 1) {
        std::cout << "Chosen GPU is " << props.deviceName << std::endl;
    }
}

QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    size_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

bool Application::isDeviceSuitable(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void Application::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(_physDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePrioriry = 1.f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePrioriry;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 {};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = deviceFeatures;

    // Enable features required for ray tracing using feature chaining via pNext
    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures {};
    enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceRayTracingFeaturesKHR enabledRayTracingFeatures {};
    enabledRayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
    enabledRayTracingFeatures.rayTracing = VK_TRUE;
    enabledRayTracingFeatures.pNext = &enabledBufferDeviceAddresFeatures;

    void* pNextChain = &enabledRayTracingFeatures;

    physicalDeviceFeatures2.pNext = pNextChain;
    createInfo.pEnabledFeatures = nullptr;
    createInfo.pNext = &physicalDeviceFeatures2;

#ifdef _DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    if (vkCreateDevice(_physDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

bool Application::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat; // Prefered format
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        VkExtent2D actualExtent = { static_cast<uint32_t>(width),
            static_cast<uint32_t>(height) };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

uint32_t Application::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkCommandBuffer Application::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Application::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_graphicsQueue);

    vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void Application::createSwapchain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = findQueueFamilies(_physDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
        indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed  to craete swap chain!");
    }

    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
    _swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapchainImages.data());

    _swapchainImageFormat = surfaceFormat.format;
    _swapchainExtent = extent;
}

void Application::createImageViews()
{
    _swapchainImageViews.resize(_swapchainImages.size());
    for (size_t i = 0; i < _swapchainImages.size(); i++) {
        _swapchainImageViews[i] = createImageView(_swapchainImages[i], _swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Application::createStorageImage()
{
    _storageImages.resize(_swapchainImages.size());
    for (size_t i = 0; i < _storageImages.size(); i++) {
        createImage(_swapchainExtent.width, _swapchainExtent.height, _swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _storageImages[i].image, _storageImages[i].memory);
        _storageImages[i].format = _swapchainImageFormat;

        _storageImages[i].view = createImageView(_storageImages[i].image, _storageImages[i].format, VK_IMAGE_ASPECT_COLOR_BIT);

        transitionImageLayout(nullptr, _storageImages[i].image, _storageImages[i].format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    }
}

void Application::createShaderBindingTable()
{
    const uint32_t groupCount = static_cast<uint32_t>(_shaderGroups.size());

    const uint32_t sbtSize = _rtHandler._rtProperties.shaderGroupBaseAlignment * groupCount;

    createBuffer(sbtSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _shaderBindingTableBuffer, _shaderBindingTableMemory);
    // Write the shader handles to the shader binding table
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    if (_rtHandler.vkGetRayTracingShaderGroupHandlesKHR(_device, _raycastPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS) {
        throw std::runtime_error("Could not Get Ray Tracing Shader Group Handles ");
    }

    uint8_t* data;
    vkMapMemory(_device, _shaderBindingTableMemory, 0, sbtSize, NULL, reinterpret_cast<void**>(&data));

    // This part is required, as the alignment and handle size may differ
    for (uint32_t i = 0; i < groupCount; i++) {
        memcpy(data, shaderHandleStorage.data() + i * _rtHandler._rtProperties.shaderGroupHandleSize, _rtHandler._rtProperties.shaderGroupHandleSize);
        data += _rtHandler._rtProperties.shaderGroupBaseAlignment;
    }
    vkUnmapMemory(_device, _shaderBindingTableMemory);
}

void Application::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uniformBufferBinding {};
    uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    uniformBufferBinding.binding = 0;
    uniformBufferBinding.descriptorCount = 1;

    VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding {};
    accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    accelerationStructureLayoutBinding.binding = 1;
    accelerationStructureLayoutBinding.descriptorCount = 1;

    VkDescriptorSetLayoutBinding resultImageLayoutBinding {};
    resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    resultImageLayoutBinding.binding = 2;
    resultImageLayoutBinding.descriptorCount = 1;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings({
        uniformBufferBinding, accelerationStructureLayoutBinding,
        resultImageLayoutBinding,
         });

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = bindings.size();
    descriptorSetLayoutCreateInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(_device, &descriptorSetLayoutCreateInfo, nullptr, &_descriptorSetLayouts.raytrace) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sert Layout!");
    }
    VkDescriptorSetLayoutBinding setLayoutBinding {};
    setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    setLayoutBinding.binding = 0;
    setLayoutBinding.descriptorCount = 1;

    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &setLayoutBinding;

    if (vkCreateDescriptorSetLayout(_device, &descriptorSetLayoutCreateInfo, nullptr, &_descriptorSetLayouts.textures) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sert Layout!");
    }
}

void Application::createGraphicsPipeline()
{
    //auto vertShader = ShaderModule(_device, "shaders/basicTriangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    //auto fragShader = ShaderModule(_device, "shaders/basicTriangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    auto raygen = ShaderModule(_device, "shaders/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    auto raymiss = ShaderModule(_device, "shaders/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
    auto raychit = ShaderModule(_device, "shaders/closehit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages({ raygen.getStageInfo(), raymiss.getStageInfo(), raychit.getStageInfo() });
    //VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader.getStageInfo(), fragShader.getStageInfo() };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptiptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptiptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptiptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //VkViewport viewport {};
    //viewport.x = 0.f;
    //viewport.y = 0.f;
    //viewport.width = static_cast<float>(_swapchainExtent.width);
    //viewport.height = static_cast<float>(_swapchainExtent.height);
    //viewport.minDepth = 0.f;
    //viewport.maxDepth = 1.f;

    //VkRect2D scissor {};
    //scissor.offset = { 0, 0 };
    //scissor.extent = _swapchainExtent;

    //VkPipelineViewportStateCreateInfo viewportState {};
    //viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    //viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport;
    //viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor;

    //VkPipelineRasterizationStateCreateInfo rasterizer {};
    //rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    //rasterizer.depthClampEnable = VK_FALSE;
    //rasterizer.rasterizerDiscardEnable = VK_FALSE;
    //rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    //rasterizer.lineWidth = 1.f;
    //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    //rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    //rasterizer.depthBiasEnable = VK_FALSE;
    //rasterizer.depthBiasConstantFactor = 0.f;
    //rasterizer.depthBiasClamp = 0.f;
    //rasterizer.depthBiasSlopeFactor = 0.f;

    //VkPipelineMultisampleStateCreateInfo multisampling {};
    //multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    //multisampling.sampleShadingEnable = VK_FALSE;
    //multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    //multisampling.minSampleShading = 1.0f;
    //multisampling.pSampleMask = nullptr;
    //multisampling.alphaToCoverageEnable = VK_FALSE;
    //multisampling.alphaToOneEnable = VK_FALSE;

    //VkPipelineColorBlendAttachmentState colorBlendAttachement {};
    //colorBlendAttachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    //colorBlendAttachement.blendEnable = VK_FALSE;
    //colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    //colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    //colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD;
    //colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    //colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    //colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD;

    //VkPipelineColorBlendStateCreateInfo colorBlending {};
    //colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    //colorBlending.logicOpEnable = VK_FALSE;
    //colorBlending.logicOp = VK_LOGIC_OP_COPY;
    //colorBlending.attachmentCount = 1;
    //colorBlending.pAttachments = &colorBlendAttachement;
    //colorBlending.blendConstants[0] = 0.f;
    //colorBlending.blendConstants[1] = 0.f;
    //colorBlending.blendConstants[2] = 0.f;
    //colorBlending.blendConstants[3] = 0.f;

    //VkPipelineDepthStencilStateCreateInfo depthStencil {};
    //depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    //depthStencil.depthTestEnable = VK_TRUE;
    //depthStencil.depthWriteEnable = VK_TRUE;
    //depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    //depthStencil.depthBoundsTestEnable = VK_FALSE;
    //depthStencil.minDepthBounds = 0.f; // Optional
    //depthStencil.maxDepthBounds = 1.f; // Optional
    //depthStencil.stencilTestEnable = VK_FALSE;
    //depthStencil.front = {}; // Optional
    //depthStencil.back = {}; // Optional

    //VkDynamicState dynamicStates[] = {
    //    VK_DYNAMIC_STATE_VIEWPORT
    //};

    //VkPipelineDynamicStateCreateInfo dynamicState {};
    //dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicState.dynamicStateCount = 0; //1;
    //dynamicState.pDynamicStates = nullptr; //dynamicStates;

    VkPushConstantRange pushConstantRange {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    constexpr uint32_t shaderIndexRaygen = 0;
    constexpr uint32_t shaderIndexMiss = 1;
    constexpr uint32_t shaderIndexClosestHit = 2;

    std::array<VkDescriptorSetLayout, 2> setLayouts = { _descriptorSetLayouts.raytrace, _descriptorSetLayouts.textures };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    VkRayTracingShaderGroupCreateInfoKHR raygenGroupCI {};
    raygenGroupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygenGroupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygenGroupCI.generalShader = shaderIndexRaygen;
    raygenGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;
    _shaderGroups.push_back(raygenGroupCI);

    VkRayTracingShaderGroupCreateInfoKHR missGroupCI {};
    missGroupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    missGroupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    missGroupCI.generalShader = shaderIndexMiss;
    missGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
    missGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
    missGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;
    _shaderGroups.push_back(missGroupCI);

    VkRayTracingShaderGroupCreateInfoKHR closesHitGroupCI {};
    closesHitGroupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    closesHitGroupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    closesHitGroupCI.generalShader = VK_SHADER_UNUSED_KHR;
    closesHitGroupCI.closestHitShader = shaderIndexClosestHit;
    closesHitGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
    closesHitGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;
    _shaderGroups.push_back(closesHitGroupCI);

    VkRayTracingPipelineCreateInfoKHR pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(_shaderGroups.size());
    pipelineInfo.pGroups = _shaderGroups.data();

    //pipelineInfo.pVertexInputState = &vertexInputInfo;
    //pipelineInfo.pInputAssemblyState = &inputAssembly;
    //pipelineInfo.pViewportState = &viewportState;
    //pipelineInfo.pRasterizationState = &rasterizer;
    //pipelineInfo.pMultisampleState = &multisampling;
    //pipelineInfo.pDepthStencilState = &depthStencil;
    //pipelineInfo.pColorBlendState = &colorBlending;
    //pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.maxRecursionDepth = 2;
    pipelineInfo.libraries.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;

    //pipelineInfo.renderPass = _renderPass;
    //pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (_rtHandler.vkCreateRayTracingPipelinesKHR(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_raycastPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create raytracing pipeline!");
    }
}

void Application::createRenderPass()
{
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = _swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    VkSubpassDependency depency {};
    depency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depency.dstSubpass = 0;
    depency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    depency.srcAccessMask = 0;
    depency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    depency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &depency;
    renderPassInfo.dependencyCount = 1;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

//void Application::createFrameBuffers()
//{
//    _swapchainFramebuffers.resize(_swapchainImageViews.size());
//
//    for (size_t i = 0; i < _swapchainImageViews.size(); i++) {
//        std::array<VkImageView, 2> attachments = {
//            _swapchainImageViews[i],
//            _depthImageView
//        };
//        VkFramebufferCreateInfo framebufferInfo {};
//        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//        framebufferInfo.renderPass = _renderPass;
//        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//        framebufferInfo.pAttachments = attachments.data();
//        framebufferInfo.width = _swapchainExtent.width;
//        framebufferInfo.height = _swapchainExtent.height;
//        framebufferInfo.layers = 1;
//
//        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapchainFramebuffers[i]) != VK_SUCCESS) {
//            throw std::runtime_error("Failed to create framebuffer!");
//        }
//    }
//}

void Application::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physDevice);

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void Application::createDepthResources()
{
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    createImage(_swapchainExtent.width, _swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage, _depthImageMemory);
    _depthImageView = createImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Yeet Rose
void Application::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
                _vertices.push_back(vertex);
            }

            _indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void Application::createUniformBuffers()
{
    VkDeviceSize BufferSize = sizeof(UniformBufferObject);

    _uniformBuffers.resize(_swapchainImages.size());
    _uniformBuffersMemory.resize(_swapchainImages.size());

    for (size_t i = 0; i < _swapchainImages.size(); i++) {
        createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);
    }
}

void Application::createDescriptorPool()
{
    VkDescriptorPoolSize uboDescriptorPoolSize {};
    uboDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorPoolSize.descriptorCount = _swapchainImages.size();

    VkDescriptorPoolSize asDescriptorPoolSize {};
    asDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    asDescriptorPoolSize.descriptorCount = _swapchainImages.size();

    VkDescriptorPoolSize storageImageDescriptorPoolSize {};
    storageImageDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    storageImageDescriptorPoolSize.descriptorCount = _swapchainImages.size();

    VkDescriptorPoolSize ImagesDescriptorPoolSize {};
    ImagesDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ImagesDescriptorPoolSize.descriptorCount = _textures.size();
    for (auto model : _models) {
        ImagesDescriptorPoolSize.descriptorCount += model._descriptorSets.size();
    }

    std::vector<VkDescriptorPoolSize> poolSizes = {
        uboDescriptorPoolSize, asDescriptorPoolSize, ImagesDescriptorPoolSize
    };
    // _swapchainImages.size() set for raytracing and one per model image/texture
    const uint32_t maxSetCount = _swapchainImages.size() + ImagesDescriptorPoolSize.descriptorCount;
    VkDescriptorPoolCreateInfo descriptorPoolInfo {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = maxSetCount;
    if (vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor pools!");
    }
}

void Application::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(_swapchainImages.size(), _descriptorSetLayouts.raytrace);
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(_swapchainImages.size());
    if (vkAllocateDescriptorSets(_device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < _swapchainImages.size(); i++) {
        VkDescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = _uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(3);
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr;
        descriptorWrites[0].pTexelBufferView = nullptr;

        VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo {};
        descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = &_rtHandler.topLevelAS.accelerationStructure;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // The specialized acceleration structure descriptor has to be chained
        descriptorWrites[1].pNext = &descriptorAccelerationStructureInfo;
        descriptorWrites[1].dstSet = _descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

        VkDescriptorImageInfo storageImageDescriptor {};
        storageImageDescriptor.imageView = _storageImages[i].view;
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = _descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = nullptr;
        descriptorWrites[2].pImageInfo = &storageImageDescriptor;
        descriptorWrites[2].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    /*std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (size_t j = 0; j < _models.size(); j++) {
        for (size_t i = 0; i < _models[j]._textures.size(); i++) {
            allocInfo.pSetLayouts = &_descriptorSetLayouts.textures;
            allocInfo.descriptorSetCount = 1;
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = _descriptorPool;
            if (vkAllocateDescriptorSets(_device, &allocInfo, &_models[j]._descriptorSets[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate descriptor sets!");
            }

            VkWriteDescriptorSet descriptorWrite = _models[j]._textures[i].getDescriptorSet(
                _models[j]._descriptorSets[i], 0);
            descriptorWrites.push_back(descriptorWrite);
        }
    }
    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);*/
}

void Application::createCommandBuffers()
{
    _commandBuffers.resize(_swapchainImages.size());

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t i = 0; i < _commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        //VkRenderPassBeginInfo renderPassInfo{};
        //renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        //renderPassInfo.renderPass = _renderPass;
        //renderPassInfo.framebuffer = _swapchainFramebuffers[i];
        //renderPassInfo.renderArea.offset = { 0, 0 };
        //renderPassInfo.renderArea.extent = _swapchainExtent;

        //std::array<VkClearValue, 2> clearValues{};
        //clearValues[0].color = { 0.f, 0.f, 0.f, 1.f };
        //clearValues[1].depthStencil = { 1.f, 0 };
        //renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        //renderPassInfo.pClearValues = clearValues.data();

        //vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raycastPipeline);
        vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipelineLayout, 0, 1, &_descriptorSets[i], 0, nullptr);

        /*VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT32);*/

        //vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

        //_models[0].draw(_commandBuffers[i], _pipelineLayout, i);

        const VkDeviceSize sbtSize = _rtHandler._rtProperties.shaderGroupBaseAlignment * (VkDeviceSize)_shaderGroups.size();
        VkStridedBufferRegionKHR raygenShaderSBTEntry {};
        raygenShaderSBTEntry.buffer = _shaderBindingTableBuffer;
        raygenShaderSBTEntry.offset = static_cast<VkDeviceSize>(_rtHandler._rtProperties.shaderGroupBaseAlignment * 0); // INDEX_RAYGEN_GROUP
        raygenShaderSBTEntry.stride = _rtHandler._rtProperties.shaderGroupBaseAlignment;
        raygenShaderSBTEntry.size = sbtSize;

        VkStridedBufferRegionKHR missShaderSBTEntry {};
        missShaderSBTEntry.buffer = _shaderBindingTableBuffer;
        missShaderSBTEntry.offset = static_cast<VkDeviceSize>(_rtHandler._rtProperties.shaderGroupBaseAlignment * 1); // INDEX_MISS_GROUP
        missShaderSBTEntry.stride = _rtHandler._rtProperties.shaderGroupBaseAlignment;
        missShaderSBTEntry.size = sbtSize;

        VkStridedBufferRegionKHR hitShaderSBTEntry {};
        hitShaderSBTEntry.buffer = _shaderBindingTableBuffer;
        hitShaderSBTEntry.offset = static_cast<VkDeviceSize>(_rtHandler._rtProperties.shaderGroupBaseAlignment * 2); // INDEX_CLOSEST_HIT_GROUP
        hitShaderSBTEntry.stride = _rtHandler._rtProperties.shaderGroupBaseAlignment;
        hitShaderSBTEntry.size = sbtSize;

        VkStridedBufferRegionKHR callableShaderSBTEntry {};

        _rtHandler.vkCmdTraceRaysKHR(
            _commandBuffers[i],
            &raygenShaderSBTEntry,
            &missShaderSBTEntry,
            &hitShaderSBTEntry,
            &callableShaderSBTEntry,
            _swapchainExtent.width,
            _swapchainExtent.height,
            1);

        // Prepare current swap chain image as transfer destination
        transitionImageLayout(_commandBuffers[i], _swapchainImages[i], _swapchainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Prepare ray tracing output image as transfer source
        transitionImageLayout(_commandBuffers[i], _storageImages[i].image, _storageImages[i].format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        VkImageCopy copyRegion {};
        copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.srcOffset = { 0, 0, 0 };
        copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.dstOffset = { 0, 0, 0 };
        copyRegion.extent = { _swapchainExtent.width, _swapchainExtent.height, 1 };
        vkCmdCopyImage(_commandBuffers[i], _storageImages[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Transition swap chain image back for presentation
        transitionImageLayout(_commandBuffers[i], _swapchainImages[i], _swapchainImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // Transition ray tracing output image back to general layout
        transitionImageLayout(_commandBuffers[i], _storageImages[i].image, _storageImages[i].format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

        //vkCmdEndRenderPass(_commandBuffers[i]);

        if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

void Application::createSemaphores()
{
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create semaphores for a frame!");
        }
    }
}

void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

  

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

      VkMemoryAllocateFlagsInfoKHR allocFlagsInfo {};
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocInfo.pNext = &allocFlagsInfo;
    }

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

VkImageView Application::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType)
{
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void Application::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(_device, image, imageMemory, 0);
}

void Application::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    bool isSingleCommandBuffer = false;
    if (!commandBuffer) {
        isSingleCommandBuffer = true;
        commandBuffer = beginSingleTimeCommands();
    }

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_GENERAL:
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        barrier.srcAccessMask = NULL;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        throw std::invalid_argument("unsupported layout transition!");
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout) {
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
    case VK_IMAGE_LAYOUT_GENERAL:
        barrier.dstAccessMask = NULL;   
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (barrier.srcAccessMask == 0) {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        throw std::invalid_argument("unsupported layout transition!");
        break;
    }

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 1, &barrier); // TODO :FIX STAGE

    if (isSingleCommandBuffer) {
        endSingleTimeCommands(commandBuffer);
    }
}

void Application::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void Application::recreateSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    //createFrameBuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}

void Application::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    //ubo.model = glm::rotate(glm::mat4(1.f), glm::radians(45.f), glm::vec3(0.f, 0.f, 1.f));
    //ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.model = glm::mat4({ 1.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f }, { 0.f, -1.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 1.f });
    ubo.view = _character.getViewMatrix();
    ubo.proj = glm::perspective(glm::radians(80.f), _swapchainExtent.width / (float)_swapchainExtent.height, 0.1f, 200.f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(_device, _uniformBuffersMemory[currentImage], 0, sizeof(ubo), NULL, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(_device, _uniformBuffersMemory[currentImage]);
}

void Application::cleanupSwapchain()
{
    vkDestroyImageView(_device, _depthImageView, nullptr);
    vkDestroyImage(_device, _depthImage, nullptr);
    vkFreeMemory(_device, _depthImageMemory, nullptr);

    for (auto framebuffer : _swapchainFramebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }

    for (size_t i = 0; i < _swapchainImages.size(); i++) {
        vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
        vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

    vkDestroyPipeline(_device, _raycastPipeline, nullptr);
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);

    for (auto imageView : _swapchainImageViews) {
        vkDestroyImageView(_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}

#ifdef _DEBUG
void Application::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

    if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}
#endif
