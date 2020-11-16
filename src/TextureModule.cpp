#include "TextureModule.hpp"
#include "Application.hpp"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

TextureModule::TextureModule(Application& app, SamplerModule& sampler)
    : _app(app)
    , _sampler(sampler)
{
}

TextureModule::TextureModule(Application& app, SamplerModule& sampler, const std::string& filename, VkImageViewType viewType)
    : _app(app)
    , _sampler(sampler)
{
    loadFromFile(filename, viewType);
}

TextureModule::~TextureModule()
{
    if (_loaded) {

        vkDestroyImageView(_app._device, _textureImageView, nullptr);

        vkDestroyImage(_app._device, _textureImage, nullptr);
        vkFreeMemory(_app._device, _textureImageMemory, nullptr);
    }
}

VkWriteDescriptorSet TextureModule::getDescriptorSet(VkDescriptorSet dst, uint32_t binding)
{
    VkWriteDescriptorSet descriptorSet {};

    _imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    _imageInfo.imageView = _textureImageView;
    _imageInfo.sampler = _sampler.getSampler();

    descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSet.dstSet = dst;
    descriptorSet.dstBinding = binding;
    descriptorSet.dstArrayElement = 0;
    descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSet.descriptorCount = 1;
    descriptorSet.pImageInfo = &_imageInfo;
    descriptorSet.pBufferInfo = nullptr;
    descriptorSet.pTexelBufferView = nullptr;

    return descriptorSet;
}

void TextureModule::loadFromFile(const std::string& filename, VkImageViewType viewType)
{
    loadTexture(filename, viewType);
}

void TextureModule::loadFromBuffer(stbi_uc* pixels, uint32_t texWidth, uint32_t texHeight, VkImageViewType viewType)
{
    assert(pixels);

    _width = texWidth;
    _height = texHeight;
    VkDeviceSize imageSize = _width * _height * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    _app.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(_app._device, stagingBufferMemory, 0, imageSize, NULL, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(_app._device, stagingBufferMemory);

    _app.createImage(_width, _height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);

    _app.transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    _app.copyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(_width), static_cast<uint32_t>(_height));

    _app.transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(_app._device, stagingBuffer, nullptr);
    vkFreeMemory(_app._device, stagingBufferMemory, nullptr);

    _textureImageView = _app.createImageView(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, viewType);
    _loaded = true;
}

void TextureModule::loadTexture(const std::string& filename, VkImageViewType viewType)
{
    stbi_uc* pixels = stbi_load(filename.c_str(), &_width, &_height, &_nbChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }
    loadFromBuffer(pixels, _width, _height, viewType);
    stbi_image_free(pixels);
}

SamplerModule::SamplerModule(Application& app)
    : _app(app)
{
    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.f;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 0.f;

    if (vkCreateSampler(_app._device, &samplerInfo, nullptr, &_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler");
    }
}

SamplerModule::~SamplerModule()
{
    vkDestroySampler(_app._device, _textureSampler, nullptr);
}

VkSampler SamplerModule::getSampler()
{
    return _textureSampler;
}
