#include "ShaderModule.hpp"

#include <fstream> // TODO : Get this in an other file (Shader Loading)

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

ShaderModule::ShaderModule(VkDevice& device, const std::string& filename, VkShaderStageFlagBits shaderStage)
    : _device(device)
    , _filename(filename)
{
    _module = createShaderModule(readFile(filename));

    _stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    _stageInfo.stage = shaderStage;
    _stageInfo.module = _module;
    _stageInfo.pName = "main";
}

ShaderModule::~ShaderModule()
{
    vkDestroyShaderModule(_device, _module, nullptr);
}

const VkPipelineShaderStageCreateInfo& ShaderModule::getStageInfo() const
{
    return _stageInfo;
}

VkShaderModule ShaderModule::createShaderModule(const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a shader module!");
    }

    return shaderModule;
}