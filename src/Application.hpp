#pragma once

#include "Utils.hpp"
#include "Character.hpp"
#include "TextureModule.hpp"
#include "gltfLoader.hpp"


#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_beta.h>

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;
constexpr size_t MAX_FRAMES_IN_FLIGHT = 6;

const std::string MODEL_PATH = "../../assets/models/peach_castle/scene.gltf";
const std::string TEXTURE_PATH = "../../assets/textures/colorful_studio_2k.hdr";

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_EXTENSION_NAME,
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, 
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, 
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, 
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Application {
public:
    void run();

    static void setVerbose(uint8_t verboseMode);

    static uint8_t getVerbose();

    void setFrameBufferResize();

    Character& getCharacter();

private:
    void drawFrame();

    void initWindow();

    void initVulkan();

    void initRayTracing();

    void createVKInstance();

    void mainLoop();

    void cleanup();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapchain();

    void createImageViews();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createRenderPass();

    void createFrameBuffers();

    void createCommandPool();

    void createDepthResources();

    void loadModel();

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffers();

    void createSemaphores();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void recreateSwapchain();

    void updateUniformBuffer(uint32_t currentImage);

    void cleanupSwapchain();

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    bool isDeviceSuitable(VkPhysicalDevice device) const;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentMode) const;

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

#ifdef _DEBUG
    void setupDebugMessenger();
#endif

private:
    // Creation Variables
    GLFWwindow* _window { nullptr };
    VkInstance _instance;
    VkSurfaceKHR _surface;
    VkPhysicalDevice _physDevice { VK_NULL_HANDLE };
    VkDevice _device;


    Character _character;
    std::vector<TextureModule> _textures;
    std::vector<SamplerModule> _samplers;
    std::vector<GltfLoader> _models;

    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSwapchainKHR _swapchain;

    VkRenderPass _renderPass;
    struct DescriptorSetLayouts {
        VkDescriptorSetLayout matrices;
        VkDescriptorSetLayout textures;
    } _descriptorSetLayouts;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;

    // Parameters
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;

    // Runtime Variables
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    std::vector<VkFramebuffer> _swapchainFramebuffers;


    // Main Loop Variables
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    size_t _currentFrame = 0;

    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    //VkBuffer _vertexBuffer;
    //VkDeviceMemory _vertexBufferMemory;
    //VkBuffer _indexBuffer;
    //VkDeviceMemory _indexBufferMemory;

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;

    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;

    bool _framebufferResized = false;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    static uint8_t _verbose;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT _debugMessenger;
#endif

    friend class TextureModule;
    friend class SamplerModule;
    friend class GltfLoader;
    friend class RaytracingHandler;
};