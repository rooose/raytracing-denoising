#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <array>

struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 materialId;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && materialId == other.materialId;
    }
};

struct Light {
    glm::vec3 pos;
    glm::vec4 color;
    float intensity;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 invView;
    glm::mat4 invProj;
    glm::uint32 vertexSize;
    glm::vec4 lights[4];
};