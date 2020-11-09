#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

class ShaderModule {
public:
    ShaderModule(VkDevice& device, const std::string& filename, VkShaderStageFlagBits shaderStage);
    ~ShaderModule();
    const VkPipelineShaderStageCreateInfo& getStageInfo() const;

private:
    VkShaderModule createShaderModule(const std::vector<char>& code) const;

private:
    VkPipelineShaderStageCreateInfo _stageInfo {};
    VkShaderModule _module;
    std::string _filename;
    VkDevice& _device;
};