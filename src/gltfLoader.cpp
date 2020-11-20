
#include "Application.hpp"

#include "gltfLoader.hpp"
#include <tiny_gltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <iostream>

GltfLoader::GltfLoader(Application& app)
    : _app(app)
{
    //_samplers.emplace_back(app);
}

GltfLoader::~GltfLoader()
{
    if (_loaded) {
        vkDestroyBuffer(_app._device, _vertices.buffer, nullptr);
        vkFreeMemory(_app._device, _vertices.memory, nullptr);
        vkDestroyBuffer(_app._device, _indices.buffer, nullptr);
        vkFreeMemory(_app._device, _indices.memory, nullptr);
    }
}

tinygltf::Model& GltfLoader::loadModel(const std::string& fileName)
{
    bool ret = _loader.LoadASCIIFromFile(&_model, &_err, &_warn, fileName);

    if (!_warn.empty()) {
        printf("Warn: %s\n", _warn.c_str());
    }

    if (!_err.empty()) {
        printf("Err: %s\n", _err.c_str());
    }

    if (!ret) {
        throw std::runtime_error("Failed to parse glTF\n");
    }

    _descriptorSets.resize(_model.images.size());
    _textures.resize(_model.images.size(), TextureModule(_app, _app._samplers[0]));
}

void GltfLoader::loadImages(tinygltf::Model& input)
{
    // Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
    // loading them from disk, we fetch them from the glTF loader and upload the buffers
    for (size_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image& glTFImage = input.images[i];
        // Get the image data from the glTF loader
        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        if (glTFImage.component == 3) {
            bufferSize = glTFImage.width * glTFImage.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb = &glTFImage.image[0];
            for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        } else {
            buffer = &glTFImage.image[0];
            bufferSize = glTFImage.image.size();
        }
        // Load texture from image buffer
        _textures[i].loadFromBuffer(buffer, glTFImage.width, glTFImage.height);
        if (deleteBuffer) {
            delete buffer;
        }
    }
}

void GltfLoader::load(std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
    loadImages(_model);
    loadMaterials(_model);
    loadTextures(_model);
    const tinygltf::Scene& scene = _model.scenes[0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = _model.nodes[scene.nodes[i]];
        loadNode(node, _model, nullptr, indexBuffer, vertexBuffer);
    }

    // Create and upload vertex and index buffer
    // We will be using one single vertex buffer and one single index buffer for the whole glTF scene
    // Primitives (of the glTF model) will then index into these using index offsets

    size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
    _indices.count = static_cast<uint32_t>(indexBuffer.size());

    VkBuffer vertexstagingBuffer;
    VkDeviceMemory vertexstagingBufferMemory;
    _app.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexstagingBuffer, vertexstagingBufferMemory);

    void* data;
    vkMapMemory(_app._device, vertexstagingBufferMemory, 0, vertexBufferSize, NULL, &data);
    memcpy(data, vertexBuffer.data(), static_cast<size_t>(vertexBufferSize));
    vkUnmapMemory(_app._device, vertexstagingBufferMemory);

    _app.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertices.buffer, _vertices.memory);

    _app.copyBuffer(vertexstagingBuffer, _vertices.buffer, vertexBufferSize);

    vkDestroyBuffer(_app._device, vertexstagingBuffer, nullptr);
    vkFreeMemory(_app._device, vertexstagingBufferMemory, nullptr);

    VkBuffer indexstagingBuffer;
    VkDeviceMemory indexstagingBufferMemory;
    _app.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexstagingBuffer, indexstagingBufferMemory);

    vkMapMemory(_app._device, indexstagingBufferMemory, 0, indexBufferSize, NULL, &data);
    memcpy(data, indexBuffer.data(), static_cast<size_t>(indexBufferSize));
    vkUnmapMemory(_app._device, indexstagingBufferMemory);

    _app.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indices.buffer, _indices.memory);

    _app.copyBuffer(indexstagingBuffer, _indices.buffer, indexBufferSize);

    vkDestroyBuffer(_app._device, indexstagingBuffer, nullptr);
    vkFreeMemory(_app._device, indexstagingBufferMemory, nullptr);
    _loaded = true;
}

void GltfLoader::loadMaterials(tinygltf::Model& input)
{
    _materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        // We only read the most basic properties required for our sample
        tinygltf::Material glTFMaterial = input.materials[i];
        // Get the base color factor
        if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
            _materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }
        // Get base color texture index
        if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
            _materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
        }
    }
}

void GltfLoader::loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, GltfLoader::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
    GltfLoader::Node* nodePtr {};
    if (parent) {
        parent->children.emplace_back(new GltfLoader::Node());
        nodePtr = parent->children.back().get();
        nodePtr->parent = parent;
    } else {
        _nodes.emplace_back(new GltfLoader::Node());
        nodePtr = _nodes.back().get();
        nodePtr->parent = nullptr;
    }
    GltfLoader::Node& node = *nodePtr;
    node.matrix = glm::mat4(1.f);

    // Get the local node matrix
    // It's either made up from translation, rotation, scale or a 4x4 matrix
    if (inputNode.translation.size() == 3) {
        node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if (inputNode.rotation.size() == 4) {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        node.matrix *= glm::mat4(q);
    }
    if (inputNode.scale.size() == 3) {
        node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    if (inputNode.matrix.size() == 16) {
        node.matrix = glm::make_mat4x4(inputNode.matrix.data());
    };

    // Load node's children
    if (inputNode.children.size() > 0) {
        for (size_t i = 0; i < inputNode.children.size(); i++) {
            loadNode(input.nodes[inputNode.children[i]], input, nodePtr, indexBuffer, vertexBuffer);
        }
    }

    // If the node contains mesh data, we load vertices and indices from the buffers
    // In glTF this is done via accessors and buffer views
    if (inputNode.mesh > -1) {
        const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
        // Iterate through all primitives of this node's mesh
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
            uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;
            // Vertices
            {
                const float* positionBuffer = nullptr;
                const float* normalsBuffer = nullptr;
                const float* texCoordsBuffer = nullptr;
                size_t vertexCount = 0;

                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++) {
                    Vertex vert {};
                    vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.texCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color = glm::vec3(1.0f);
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            {
                const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                // glTF supports different component types of indices
                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    uint32_t* buf = new uint32_t[accessor.count];
                    memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    uint16_t* buf = new uint16_t[accessor.count];
                    memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    uint8_t* buf = new uint8_t[accessor.count];
                    memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }
            Primitive primitive {};
            primitive.firstIndex = firstIndex;
            primitive.indexCount = indexCount;
            primitive.materialIndex = glTFPrimitive.material;
            node.mesh.primitives.push_back(primitive);
        }
    }
}

void GltfLoader::loadTextures(tinygltf::Model& input)
{
    _textures_idx.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        _textures_idx[i].imageIndex = input.textures[i].source;
    }
}

// Draw a single node including child nodes (if present)
void GltfLoader::drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame, GltfLoader::Node node)
{
    VkDescriptorSet& descriptorSet = _app._descriptorSets[currentFrame];
    if (node.mesh.primitives.size() > 0) {
        // Pass the node's matrix via push constants
        // Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
        glm::mat4 nodeMatrix = node.matrix;
        GltfLoader::Node* currentParent = node.parent;
        while (currentParent) {
            nodeMatrix = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }

        VkDescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = _app._uniformBuffers[currentFrame];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        // Pass the final matrix to the vertex shader using push constants
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
        for (const GltfLoader::Primitive& primitive : node.mesh.primitives) {
            if (primitive.indexCount > 0) {
                // Get the texture index for this primitive
                Texture texture = _textures_idx[_materials[primitive.materialIndex].baseColorTextureIndex];
                // Bind the descriptor for the current primitive's texture
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_descriptorSets[texture.imageIndex], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (auto& child : node.children) {
        drawNode(commandBuffer, pipelineLayout, currentFrame, *child.get());
    }
}

// Draw the glTF scene starting at the top-level-nodes
void GltfLoader::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame)
{
    // All vertices and indices are stored in single buffers, so we only need to bind once
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertices.buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    // Render all nodes at top-level
    for (auto& node : _nodes) {
        drawNode(commandBuffer, pipelineLayout, currentFrame, *node.get());
    }
}