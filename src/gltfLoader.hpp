#pragma once

#include "TextureModule.hpp"
#include "Utils.hpp"
#include "tiny_gltf.h"

class Application;

class GltfLoader {
public:
    struct Material {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        uint32_t baseColorTextureIndex;
    };

    // A primitive contains the data for a single draw call
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex;
    };

    // Contains the node's (optional) geometry and can be made up of an arbitrary number of primitives
    struct Mesh {
        std::vector<Primitive> primitives;
    };

    // A node represents an object in the glTF scene graph
    struct Node {
        Node* parent;
        std::vector<Node> children;
        Mesh mesh;
        glm::mat4 matrix;
    };

    struct Texture { // TODO : Put this in TextureModule
        int32_t imageIndex;
    };

public:
    GltfLoader(Application& app);
    ~GltfLoader();

    tinygltf::Model& loadModel(const std::string& fileName, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
    void loadImages(tinygltf::Model& input);
    void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, GltfLoader::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame, GltfLoader::Node node);
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame);

private:
    void loadMaterials(tinygltf::Model& input);
    void loadTextures(tinygltf::Model& input);

private:
    std::vector<TextureModule> _textures;
    std::vector<Texture> _textures_idx;
    std::vector<SamplerModule> _samplers;
    std::vector<Material> _materials;
    std::vector<Node> _nodes;

    Application& _app;

    // Single vertex buffer for all primitives
    struct {
        VkBuffer buffer;
        VkDeviceMemory memory;
    } _vertices;

    // Single index buffer for all primitives
    struct {
        int count;
        VkBuffer buffer;
        VkDeviceMemory memory;
    } _indices;

private:
    tinygltf::Model _model;
    tinygltf::TinyGLTF _loader;
    std::string _err;
    std::string _warn;
};