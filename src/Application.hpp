#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;

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
    /// <summary>
    /// Set verbose level for the application
    /// </summary>
    /// <param name="verboseMode"> Verbose Level [0,3]</param>
    static void setVerbose(uint8_t verboseMode);
    /// <summary>
    /// Get Verbose of the application
    /// </summary>
    /// <returns>Verbose Mode</returns>
    static uint8_t getVerbose();

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

    void createSwapChain();

    void createImageViews();

    void createGraphicsPipeline();

    void createRenderPass();

    void createFramBuffers();

    void createCommandPool();

    void createCommandBuffers();

    void createSemaphores();

    VkShaderModule createShaderModule(const std::vector<char>& code) const; // TODO : Get this in an other file

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    bool isDeviceSuitable(VkPhysicalDevice device) const;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentMode) const;

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

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

    VkQueue _graphicQueue;
    VkQueue _presentQueue;
    VkSwapchainKHR _swapchain;

    VkRenderPass _renderPass;
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
    VkSemaphore _imageAvailableSemaphore;
    VkSemaphore _renderFinishedSemaphore;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;

    static uint8_t _verbose;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT _debugMessenger;
#endif
};