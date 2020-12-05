#include "RandomScene.hpp"

#include <random>
#include <time.h>
#include <vector>

static std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> getDefaultCube()
{
    std::array<uint32_t, 36> indices = {
        0, 1, 2, 0, 2, 3, // front
        4, 5, 6, 4, 6, 7, // back
        8, 9, 10, 8, 10, 11, // top
        12, 13, 14, 12, 14, 15, // bottom
        16, 17, 18, 16, 18, 19, // right
        20, 21, 22, 20, 22, 23, // left
    };

    std::array<glm::vec3, 24> vertices = {
        // Front face
        glm::vec3(-1.0, -1.0, 1.0),
        glm::vec3(-1.0, 1.0, 1.0),
        glm::vec3(1.0, 1.0, 1.0),
        glm::vec3(1.0, -1.0, 1.0),

        // Back face
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3(1.0, -1.0, -1.0),
        glm::vec3(1.0, 1.0, -1.0),
        glm::vec3(-1.0, 1.0, -1.0),

        // Top face
        glm::vec3(-1.0, 1.0, -1.0),
        glm::vec3(1.0, 1.0, -1.0),
        glm::vec3(1.0, 1.0, 1.0),
        glm::vec3(-1.0, 1.0, 1.0),

        // Bottom face
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3(-1.0, -1.0, 1.0),
        glm::vec3(1.0, -1.0, 1.0),
        glm::vec3(1.0, -1.0, -1.0),

        // Right face
        glm::vec3(1.0, -1.0, -1.0),
        glm::vec3(1.0, -1.0, 1.0),
        glm::vec3(1.0, 1.0, 1.0),
        glm::vec3(1.0, 1.0, -1.0),

        // Left face
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3(-1.0, 1.0, -1.0),
        glm::vec3(-1.0, 1.0, 1.0),
        glm::vec3(-1.0, -1.0, 1.0),
    };

    return std::make_pair(std::vector(vertices.begin(), vertices.end()), std::vector(indices.begin(), indices.end()));
}

static std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> getDefaultPlan()
{
    std::array<uint32_t, 6> indices = {
        1, 2, 0, 3, 0, 2
    };

    std::array<glm::vec3, 4> vertices = {
        glm::vec3(-0.5f, -0.5f, 0),
        glm::vec3(-0.5f, 0.5f, 0),
        glm::vec3(0.5f, 0.5f, 0),
        glm::vec3(0.5f, -0.5f, 0),
    };

    return std::make_pair(std::vector(vertices.begin(), vertices.end()), std::vector(indices.begin(), indices.end()));
}

static std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> getDefaultIcosahedron()
{
    const double sqr5 = sqrt(5.0);
    const double phi = (1.0 + sqr5) * 0.5;

    const double ratio = sqrt(10.0 + (2.0 * sqr5)) / (4.0 * phi);
    const float a = (1.f / ratio) * 0.5f;
    const float b = (1.f / ratio) / (2.f * phi);

    std::array<uint32_t, 60> indices = {
        0, 1, 2, //0
        3, 2, 1, //1
        3, 4, 5, //2
        3, 8, 4, //3
        0, 6, 7, //4
        0, 9, 6, //5
        4, 10, 11, //6
        6, 11, 10, //7
        2, 5, 9, //8
        11, 9, 5, //9
        1, 7, 8, //10
        10, 8, 7, //11
        3, 5, 2, //12
        3, 1, 8, //13
        0, 2, 9, //14
        0, 7, 1, //15
        6, 9, 11, //16
        6, 10, 7, //17
        4, 11, 5, //18
        4, 8, 10, //19
    };

    std::array<glm::vec3, 12> vertices = {
        glm::vec3(0.f, b, -a),
        glm::vec3(b, a, 0.f),
        glm::vec3(-b, a, 0.f),
        glm::vec3(0.f, b, a),
        glm::vec3(0.f, -b, a),
        glm::vec3(-a, 0.f, b),
        glm::vec3(0.f, -b, -a),
        glm::vec3(a, 0.f, -b),
        glm::vec3(a, 0.f, b),
        glm::vec3(-a, 0.f, -b),
        glm::vec3(b, -a, 0.f),
        glm::vec3(-b, -a, 0.f)
    };

    return std::make_pair(std::vector(vertices.begin(), vertices.end()), std::vector(indices.begin(), indices.end()));
}

static std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> tesselateIcosahedron(std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> icosahedron)
{
    std::vector<uint32_t> indices;
    indices.resize(icosahedron.second.size() * 4);
    std::vector<glm::vec3> vertices(icosahedron.first.begin(), icosahedron.first.end());

    for (size_t i = 0; i < icosahedron.second.size(); i += 3) {
        std::array<glm::vec3, 6> v;
        std::array<uint32_t, 6> idx;
        idx[0] = icosahedron.second[i + 0];
        idx[1] = icosahedron.second[i + 1];
        idx[2] = icosahedron.second[i + 2];
        idx[3] = vertices.size() + 0;
        idx[4] = vertices.size() + 1;
        idx[5] = vertices.size() + 2;

        v[0] = icosahedron.first[idx[0]];
        v[1] = icosahedron.first[idx[1]];
        v[2] = icosahedron.first[idx[2]];
        v[3] = glm::normalize(0.5f * (v[0] + v[1]));
        v[4] = glm::normalize(0.5f * (v[1] + v[2]));
        v[5] = glm::normalize(0.5f * (v[2] + v[0]));

        vertices.push_back(v[3]);
        vertices.push_back(v[4]);
        vertices.push_back(v[5]);

        std::array<uint32_t, 12> newIndices = {
            idx[0], idx[3], idx[5], // 1
            idx[3], idx[1], idx[4], // 1
            idx[4], idx[2], idx[5], // 1
            idx[3], idx[4], idx[5], // 1
        };

        indices.insert(indices.end(), newIndices.begin(), newIndices.end());
    }

    return std::make_pair(vertices, indices);
}

static std::vector<glm::vec3> computeNormals(std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> verticesData)
{
    std::vector<glm::vec3> normals;
    std::vector<size_t> vertexGeoCount;
    normals.resize(verticesData.first.size());
    vertexGeoCount.resize(verticesData.first.size());

    for (size_t i = 0; i < verticesData.second.size(); i += 3) {
        std::array<glm::vec3, 3> v;
        for (size_t j = 0; j < v.size(); j++) {
            const size_t idx = verticesData.second[j + i];
            v[j] = verticesData.first[idx];
        }
        // Compute normal
        glm::vec3 normal = glm::normalize(glm::cross(v[2] - v[0], v[1] - v[0]));

        // Assign normal for each vertex
        for (size_t j = 0; j < v.size(); j++) {
            const size_t idx = verticesData.second[j + i];
            normals[idx] += normal;
            vertexGeoCount[idx]++;
        }
    }

    // Normalize the normals
    for (size_t i = 0; i < normals.size(); i++) {
        normals[i] /= static_cast<float>(vertexGeoCount[i]);
    }

    return normals;
}

RandomScene::RandomScene(Application& app, float sceneSize, uint32_t scale, uint32_t seed)
    : GltfLoader(app)
    , _sceneSize(sceneSize)
{
    if (seed == -1) {
        seed = static_cast<uint32_t>(time(NULL));
    }
    // Init rand
    srand(seed);

    // Generate all items counts
    const size_t nbLights = static_cast<size_t>(rand() % (scale / 4)) + 1;
    const size_t nbMaterials = 3 * static_cast<size_t>(rand() % (scale)) + scale * 3 / 4;
    const size_t nbSpheres = static_cast<size_t>(rand() % scale) + scale / 2;
    const size_t nbBoxes = static_cast<size_t>(rand() % scale) + scale / 2;

    // Generate lighting and materials
    generateLighting(nbLights, /* hasMovement */ true);
    generateMaterials(nbMaterials);

    // Generate floor plan
    generateFloor();

    // generate all the objects
    generateSpheres(nbSpheres);
    generateBoxes(nbBoxes);
}

void RandomScene::load(std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
    indexBuffer.insert(indexBuffer.end(), _indices.begin(), _indices.end());
    vertexBuffer.insert(vertexBuffer.end(), _vertices.begin(), _vertices.end());
    createBuffers(indexBuffer, vertexBuffer);
}

void RandomScene::updateLights(float deltaTime)
{
    for (size_t i = 0; i < _lights.size(); i++) {
        _lights[i].pos = glm::rotate(glm::identity<glm::mat4>(), deltaTime * _LightMouvement[i].first, _LightMouvement[i].second) * glm::vec4(_lights[i].pos, 0.);
    }
}

RandomScene::~RandomScene()
{
}

void RandomScene::update(float deltaTime)
{
    updateLights(deltaTime);
}

void RandomScene::generateLighting(size_t nbLight, bool hasMovement)
{
    _LightMouvement.resize(nbLight, std::make_pair(0, glm::vec3(1., 0., 0.)));
    // Possibly generate different type of lights
    // Generate nb of lights and applie transform
    for (size_t i = 0; i < nbLight; i++) {
        if (hasMovement) {
            // set speed
            _LightMouvement[i].first = glm::radians(static_cast<float>(rand() % 500) / 100000.);
            _LightMouvement[i].second = glm::normalize(glm::vec3(static_cast<float>(rand() % 10000) / 10000., static_cast<float>(rand() % 10000) / 10000., static_cast<float>(rand() % 10000) / 10000.));
        }
        Light light;
        // Generate random color
        light.color = glm::vec4(static_cast<float>(rand() % 10000) / 10000.f, static_cast<float>(rand() % 10000) / 10000.f, static_cast<float>(rand() % 10000) / 10000.f, 1.f);
        light.intensity = static_cast<float>(rand() % 5000) / 5000.f + .5f;
        light.pos = getRandomTransformation() * glm::vec4(0.f, 0.f, 0.f, 1.f);
        _lights.push_back(light);
    }
}

void RandomScene::generateMaterials(size_t nbMaterials)
{
    // Hard code a mirror
    {
        Material mirrorMat;
        mirrorMat.baseColorFactor = glm::vec4(1.f);
        mirrorMat.reflexionCoeff = 1.f;
        _materials.push_back(mirrorMat);
    }

    // Generate random Base color
    for (size_t i = 0; i < nbMaterials; i++) {
        Material mat;
        mat.baseColorFactor = glm::vec4(static_cast<float>(rand() % 10000) / 10000.f, static_cast<float>(rand() % 10000) / 10000.f, static_cast<float>(rand() % 10000) / 10000.f, 1.f);
        mat.baseColorTextureIndex = -1;
        mat.normalTextureIndex = -1;
        mat.ambientCoeff = static_cast<float>(rand() % 50000) / 10000.f + 0.5;
        mat.diffuseCoeff = static_cast<float>(rand() % 50000) / 10000.f + 0.5;
        mat.reflexionCoeff = static_cast<float>(rand() % 10000) / 10000.f;
        mat.shininessCoeff = static_cast<float>(rand() % 20);
        mat.specularCoeff = static_cast<float>(rand() % 50000) / 10000.f + 0.5;
        ;

        _materials.push_back(mat);
    }

    // Generate all the other material features
}

void RandomScene::generateSpheres(size_t nbSpheres)
{
    // Generate Icosahedron
    auto simpleIcosahedron = getDefaultIcosahedron();

    // Tesselate Icosahedron
    constexpr size_t nbTesselation = 5;
    for (size_t i = 0; i < nbTesselation; i++) {
        simpleIcosahedron = tesselateIcosahedron(simpleIcosahedron);
    }

    const auto normals = computeNormals(simpleIcosahedron);

    for (size_t i = 0; i < nbSpheres; i++) {

        // Applie random scale trasform (ratio only)
        const glm::vec3 ratios = { 1.f, static_cast<float>(rand() % 400) / 100. + 0.8f, static_cast<float>(rand() % 400) / 100. + 0.8f };
        const glm::mat4 scaleMat = glm::scale(glm::identity<glm::mat4>(), glm::normalize(ratios));

        // applie random transfom
        const auto randomTransform = getRandomTransformation();

        // Create Actual Data
        const size_t randomMaterialId = rand() % _materials.size();

        // Create Actual Data
        size_t startVertexCount = _vertices.size();
        size_t startIndicesCount = _indices.size();

        std::vector<Vertex> vertices;
        vertices.resize(simpleIcosahedron.first.size());

        for (size_t j = 0; j < vertices.size(); j++) {
            Vertex& vertex = vertices[j];
            vertex.color = glm::vec4(1.f);
            vertex.pos = randomTransform * scaleMat * glm::vec4(simpleIcosahedron.first[j], 1.f);
            vertex.normal = randomTransform * glm::vec4(normals[j], 0.f);
            vertex.texCoord = glm::vec2(0.f);
            vertex.materialId = glm::vec4(randomMaterialId);
        }

        std::vector<uint32_t> indices;
        indices.resize(simpleIcosahedron.second.size());
        for (size_t j = 0; j < indices.size(); j++) {
            indices[j] = simpleIcosahedron.second[j] + startVertexCount;
        }

        _vertices.insert(_vertices.end(), vertices.begin(), vertices.end());
        _indices.insert(_indices.end(), indices.begin(), indices.end());
    }
}

void RandomScene::generateBoxes(size_t nbBoxes)
{

    // Create simple squarebox
    const auto simpleBox = getDefaultCube();
    for (size_t i = 0; i < nbBoxes; i++) {
        // Applie random scale trasform (ratio only)
        const glm::vec3 ratios = { 1.f, static_cast<float>(rand() % 1000) / 100. + 0.5, static_cast<float>(rand() % 1000) / 100. + 0.5 };
        const glm::mat4 scaleMat = glm::scale(glm::identity<glm::mat4>(), glm::normalize(ratios));
        const auto normals = computeNormals(simpleBox);
        const auto randomTransform = getRandomTransformation();

        const size_t randomMaterialId = rand() % _materials.size();

        // Create Actual Data
        size_t startVertexCount = _vertices.size();
        size_t startIndicesCount = _indices.size();

        std::vector<Vertex> vertices;
        vertices.resize(simpleBox.first.size());

        for (size_t j = 0; j < vertices.size(); j++) {
            Vertex& vertex = vertices[j];
            vertex.color = glm::vec4(1.f);
            vertex.pos = randomTransform * scaleMat * glm::vec4(simpleBox.first[j], 1.f);
            vertex.normal = randomTransform * glm::vec4(normals[j], 0.f);
            vertex.texCoord = glm::vec2(0.f);
            vertex.materialId = glm::vec4(randomMaterialId);
        }

        std::vector<uint32_t> indices;
        indices.resize(simpleBox.second.size());
        for (size_t j = 0; j < indices.size(); j++) {
            indices[j] = simpleBox.second[j] + startVertexCount;
        }

        _vertices.insert(_vertices.end(), vertices.begin(), vertices.end());
        _indices.insert(_indices.end(), indices.begin(), indices.end());
    }
}

void RandomScene::generateFloor()
{
    // Create Simple plan (I'm just a kid and my life is a nightmare)
    const auto simplePlan = getDefaultPlan();
    const auto normals = computeNormals(simplePlan);

    // Scale it to _sceneSize
    const auto scaleMat = glm::scale(glm::identity<glm::mat4>(), glm::vec3(_sceneSize * 2));

    const size_t randomMaterialId = rand() % _materials.size();

    // Create Actual Data
    size_t startVertexCount = _vertices.size();
    size_t startIndicesCount = _indices.size();

    std::vector<Vertex> vertices;
    vertices.resize(simplePlan.first.size());

    for (size_t j = 0; j < vertices.size(); j++) {
        Vertex& vertex = vertices[j];
        vertex.color = glm::vec4(1.f);
        vertex.pos = scaleMat * glm::vec4(simplePlan.first[j], 1.f);
        vertex.normal = normals[j];
        vertex.texCoord = glm::vec2(0.f);
        vertex.materialId = glm::vec4(randomMaterialId);
    }

    std::vector<uint32_t> indices;
    indices.resize(simplePlan.second.size());
    for (size_t j = 0; j < indices.size(); j++) {
        indices[j] = simplePlan.second[j] + startVertexCount;
    }

    _vertices.insert(_vertices.end(), vertices.begin(), vertices.end());
    _indices.insert(_indices.end(), indices.begin(), indices.end());
}

glm::mat4 RandomScene::getRandomTransformation() const
{
    // Get random Rotate
    glm::mat4 transform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(static_cast<float>(rand() % 360)), glm::normalize(glm::vec3(rand(), rand(), rand())));

    // Add Transform
    transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(fmod(rand(), _sceneSize) - _sceneSize / 2.f, fmod(rand(), _sceneSize) - _sceneSize / 2.f, fmod(rand(), _sceneSize / 2.f))) * transform;

    return transform;
}
