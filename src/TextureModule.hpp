#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <stb_image.h>

class Application;

class SamplerModule {
public:
    SamplerModule(Application& app);
    ~SamplerModule();

    VkSampler getSampler();

private:
    Application& _app;

    VkSampler _textureSampler;
};

class TextureModule {
public:
    TextureModule(Application& app, SamplerModule& sampler);
    TextureModule(Application& app, SamplerModule& sampler, const std::string& filename, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    ~TextureModule();

    VkWriteDescriptorSet getDescriptorSet(VkDescriptorSet dst, uint32_t binding);
    void loadFromFile(const std::string& filename, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    void loadFromBuffer(stbi_uc* pixels, uint32_t texWidth, uint32_t texHeight, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

private:

    void loadTexture(const std::string& filename, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

private:
    Application& _app;
    SamplerModule& _sampler;

    int _width { 0 };
    int _height { 0 };
    int _nbChannels { 0 };

    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkDescriptorImageInfo _imageInfo {};

    VkImageView _textureImageView;

    bool _loaded { false };
};