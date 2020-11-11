#pragma once

#include "Utils.hpp"
#include "Character.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;
constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

// Rose
const std::string MODEL_PATH = "../../assets/models/viking_room.obj";
const std::string TEXTURE_PATH = "../../assets/textures/viking_room.png";

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    void createFramBuffers();

    void createCommandPool();

    void createDepthResources();

    void createTextureImage();

    void createTextureImageView();

    void createTextureSampler();

    // Rose
    void loadModel();

    void createVertexBuffer();

    void createIndexBuffer();

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffers();

    void createSemaphores();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

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


    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSwapchainKHR _swapchain;

    VkRenderPass _renderPass;
    VkDescriptorSetLayout _descriptorSetLayout;
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
    //Rose
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;

    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;

    bool _framebufferResized = false;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;

    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkImageView _textureImageView;
    VkSampler _textureSampler;

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    static uint8_t _verbose;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT _debugMessenger;
#endif
};