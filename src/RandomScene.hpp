#pragma once

#include "gltfLoader.hpp"
#include <glm/glm.hpp>

#include <vector>

class RandomScene : public GltfLoader {
public:
    RandomScene(Application& app, float sceneSize, uint32_t scale = 10, uint32_t seed = -1);
    virtual void load(std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) override;
    void updateLights(float deltaTime);
    ~RandomScene();
    virtual void update(float deltaTime) override;

private:
    void generateLighting(size_t nbLight, bool hasMovement = false);
    void generateMaterials(size_t nbMaterials);
    void generateSpheres(size_t nbSpheres);
    void generateBoxes(size_t nbBoxes);
    void generateFloor();
    glm::mat4 getRandomTransformation() const;

private:
    float _sceneSize;
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;
    std::vector<std::pair<float, glm::vec3>> _LightMouvement;
};