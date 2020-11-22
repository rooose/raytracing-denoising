#include "Application.hpp"
#include "RaytracingHandler.hpp"

RaytracingHandler::RaytracingHandler(Application& app)
    : _app(app)
{
}

RaytracingHandler::~RaytracingHandler()
{ 
}

void RaytracingHandler::init() {
    _rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &_rtProperties;
    deviceProps2.properties = {};
    vkGetPhysicalDeviceProperties2(_app._physDevice, &deviceProps2);

    _rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &_rtFeatures;
    vkGetPhysicalDeviceFeatures2(_app._physDevice, &deviceFeatures2);

    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(_app._device, "vkGetBufferDeviceAddressKHR"));
    vkBindAccelerationStructureMemoryKHR = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryKHR>(vkGetDeviceProcAddr(_app._device, "vkBindAccelerationStructureMemoryKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(_app._device, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(_app._device, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureMemoryRequirementsKHR = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsKHR>(vkGetDeviceProcAddr(_app._device, "vkGetAccelerationStructureMemoryRequirementsKHR"));
    vkCmdBuildAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureKHR>(vkGetDeviceProcAddr(_app._device, "vkCmdBuildAccelerationStructureKHR"));
    vkBuildAccelerationStructureKHR = reinterpret_cast<PFN_vkBuildAccelerationStructureKHR>(vkGetDeviceProcAddr(_app._device, "vkBuildAccelerationStructureKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(_app._device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(_app._device, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_app._device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_app._device, "vkCreateRayTracingPipelinesKHR"));

    createBottomLevelAccelerationStructure();
    createTopLevelAccelerationStructure();
}

void RaytracingHandler::cleanupRaytracingHandler()
{
    vkDestroyAccelerationStructureKHR(_app._device, bottomLevelAS.accelerationStructure, nullptr);
    vkDestroyAccelerationStructureKHR(_app._device, topLevelAS.accelerationStructure, nullptr);

    deleteObjectMemory(bottomLevelAS.objectMemory);
    deleteObjectMemory(topLevelAS.objectMemory);
}

void RaytracingHandler::createBottomLevelAccelerationStructure()
{
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(_app._models[0]._vertices.buffer);
    indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(_app._models[0]._indices.buffer);

    VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationCreateGeometryInfo{};
    accelerationCreateGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
    accelerationCreateGeometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationCreateGeometryInfo.maxPrimitiveCount = static_cast<uint32_t>(_app._indices.size() / 3);
    accelerationCreateGeometryInfo.indexType = VK_INDEX_TYPE_UINT32;
    accelerationCreateGeometryInfo.maxVertexCount = static_cast<uint32_t>(_app._vertices.size());
    accelerationCreateGeometryInfo.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationCreateGeometryInfo.allowsTransforms = VK_FALSE;

    VkAccelerationStructureCreateInfoKHR accelerationCI{};
    accelerationCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationCI.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationCI.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationCI.maxGeometryCount = 1;
    accelerationCI.pGeometryInfos = &accelerationCreateGeometryInfo;

    if (vkCreateAccelerationStructureKHR(_app._device, &accelerationCI, nullptr, &bottomLevelAS.accelerationStructure) != VK_SUCCESS) {
        throw std::runtime_error("failed to create bottom level acceleration structure!");
    }

    bottomLevelAS.objectMemory = createObjectMemory(bottomLevelAS.accelerationStructure);

    VkBindAccelerationStructureMemoryInfoKHR bindAccelerationMemoryInfo{};
    bindAccelerationMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
    bindAccelerationMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
    bindAccelerationMemoryInfo.memory = bottomLevelAS.objectMemory.memory;

    if (vkBindAccelerationStructureMemoryKHR(_app._device, 1, &bindAccelerationMemoryInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind bottom level acceleration structure memory info!");
    }

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = accelerationCreateGeometryInfo.geometryType;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexBufferDeviceAddress.deviceAddress;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexBufferDeviceAddress.deviceAddress;

    std::vector<VkAccelerationStructureGeometryKHR> accelerationGeometries = { accelerationStructureGeometry };
    VkAccelerationStructureGeometryKHR* accelerationStructureGeometries = accelerationGeometries.data();

    RayTracingScratchBuffer scratchBuffer = createScratchBuffer(bottomLevelAS.accelerationStructure);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.update = VK_FALSE;
    accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.accelerationStructure;
    accelerationBuildGeometryInfo.geometryArrayOfPointers = VK_FALSE;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.ppGeometries = &accelerationStructureGeometries;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildOffsetInfoKHR accelerationBuildOffsetInfo{};
    accelerationBuildOffsetInfo.primitiveCount = accelerationCreateGeometryInfo.maxPrimitiveCount;
    accelerationBuildOffsetInfo.primitiveOffset = 0x0;
    accelerationBuildOffsetInfo.firstVertex = 0;
    accelerationBuildOffsetInfo.transformOffset = 0x0;

    std::vector<VkAccelerationStructureBuildOffsetInfoKHR*> accelerationBuildOffsets = { &accelerationBuildOffsetInfo };

    if (_rtFeatures.rayTracingHostAccelerationStructureCommands)
    {
        if (vkBuildAccelerationStructureKHR(_app._device, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Could not Create Bottom level acceleration Structure");
        }
    }
    else
    {
        VkCommandBuffer commandBuffer = _app.beginSingleTimeCommands();
        vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data());
        _app.endSingleTimeCommands(commandBuffer);
    }

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.accelerationStructure;

    bottomLevelAS.handle = vkGetAccelerationStructureDeviceAddressKHR(_app._device, &accelerationDeviceAddressInfo);

    deleteScratchBuffer(scratchBuffer);
}

void RaytracingHandler::createTopLevelAccelerationStructure()
{
    VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationCreateGeometryInfo{};
    accelerationCreateGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
    accelerationCreateGeometryInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationCreateGeometryInfo.maxPrimitiveCount = 1;
    accelerationCreateGeometryInfo.allowsTransforms = VK_FALSE;

    VkAccelerationStructureCreateInfoKHR accelerationCI{};
    accelerationCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationCI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationCI.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationCI.maxGeometryCount = 1;
    accelerationCI.pGeometryInfos = &accelerationCreateGeometryInfo;

    if (vkCreateAccelerationStructureKHR(_app._device, &accelerationCI, nullptr, &topLevelAS.accelerationStructure) != VK_SUCCESS) {
        throw std::runtime_error("Could not Create Top level acceleration Structure");
    }

    topLevelAS.objectMemory = createObjectMemory(topLevelAS.accelerationStructure);

    VkBindAccelerationStructureMemoryInfoKHR bindAccelerationMemoryInfo{};
    bindAccelerationMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
    bindAccelerationMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure;
    bindAccelerationMemoryInfo.memory = topLevelAS.objectMemory.memory;

    if (vkBindAccelerationStructureMemoryKHR(_app._device, 1, &bindAccelerationMemoryInfo) != VK_SUCCESS) {
        throw std::runtime_error("Could not bind Accleration Strucute Memory for top level acceleration");
    }

    VkTransformMatrixKHR transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f };

    VkAccelerationStructureInstanceKHR instance{};
    instance.transform = transformMatrix;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = bottomLevelAS.handle;

    VkBuffer instancesBuffer;
    VkDeviceMemory instancesBufferMemory;
    _app.createBuffer(sizeof(instance), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    instancesBuffer, instancesBufferMemory);
    
    void* data;
    vkMapMemory(_app._device, instancesBufferMemory, 0, sizeof(instance), NULL, &data);
    memcpy(data, &instance, static_cast<size_t>(sizeof(instance)));
    vkUnmapMemory(_app._device, instancesBufferMemory);

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instancesBuffer);

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data.deviceAddress = instanceDataDeviceAddress.deviceAddress;

    std::vector<VkAccelerationStructureGeometryKHR> accelerationGeometries = { accelerationStructureGeometry };
    VkAccelerationStructureGeometryKHR* accelerationStructureGeometries = accelerationGeometries.data();

    RayTracingScratchBuffer scratchBuffer = createScratchBuffer(topLevelAS.accelerationStructure);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.update = VK_FALSE;
    accelerationBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.accelerationStructure;
    accelerationBuildGeometryInfo.geometryArrayOfPointers = VK_FALSE;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.ppGeometries = &accelerationStructureGeometries;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildOffsetInfoKHR accelerationBuildOffsetInfo{};
    accelerationBuildOffsetInfo.primitiveCount = 1;
    accelerationBuildOffsetInfo.primitiveOffset = 0x0;
    accelerationBuildOffsetInfo.firstVertex = 0;
    accelerationBuildOffsetInfo.transformOffset = 0x0;
    std::vector<VkAccelerationStructureBuildOffsetInfoKHR*> accelerationBuildOffsets = { &accelerationBuildOffsetInfo };

    if (_rtFeatures.rayTracingHostAccelerationStructureCommands)
    {
        if (vkBuildAccelerationStructureKHR(_app._device, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Could not build acceleration structure for top level");
        }
    }
    else
    {
        VkCommandBuffer commandBuffer = _app.beginSingleTimeCommands();
        vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data());
        _app.endSingleTimeCommands(commandBuffer);
    }

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = topLevelAS.accelerationStructure;

    topLevelAS.handle = vkGetAccelerationStructureDeviceAddressKHR(_app._device, &accelerationDeviceAddressInfo);

    deleteScratchBuffer(scratchBuffer);
    vkDestroyBuffer(_app._device, instancesBuffer, nullptr);
    vkFreeMemory(_app._device, instancesBufferMemory, nullptr);
}

uint64_t RaytracingHandler::getBufferDeviceAddress(VkBuffer buffer)
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return this->vkGetBufferDeviceAddressKHR(_app._device, &bufferDeviceAI);
}

RayTracingObjectMemory RaytracingHandler::createObjectMemory(VkAccelerationStructureKHR accelerationStructure)
{
    RayTracingObjectMemory objectMemory{};
    VkMemoryRequirements2 memoryRequirements2{};
    memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkAccelerationStructureMemoryRequirementsInfoKHR accelerationStructureMemoryRequirements{};
    accelerationStructureMemoryRequirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
    accelerationStructureMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
    accelerationStructureMemoryRequirements.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
    accelerationStructureMemoryRequirements.accelerationStructure = accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsKHR(_app._device, &accelerationStructureMemoryRequirements, &memoryRequirements2);

    VkMemoryRequirements memoryRequirements = memoryRequirements2.memoryRequirements;

    VkMemoryAllocateInfo memoryAI{};
    memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAI.allocationSize = memoryRequirements.size;
    memoryAI.memoryTypeIndex = _app.findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(_app._device, &memoryAI, nullptr, &objectMemory.memory) != VK_SUCCESS) {
        throw std::runtime_error("Could not create object memory");
    }

    return objectMemory;
}

RayTracingScratchBuffer RaytracingHandler::createScratchBuffer(VkAccelerationStructureKHR accelerationStructure)
{
    RayTracingScratchBuffer scratchBuffer{};

    VkMemoryRequirements2 memoryRequirements2{};
    memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkAccelerationStructureMemoryRequirementsInfoKHR accelerationStructureMemoryRequirements{};
    accelerationStructureMemoryRequirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
    accelerationStructureMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
    accelerationStructureMemoryRequirements.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
    accelerationStructureMemoryRequirements.accelerationStructure = accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsKHR(_app._device, &accelerationStructureMemoryRequirements, &memoryRequirements2);

    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = memoryRequirements2.memoryRequirements.size;
    bufferCI.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_app._device, &bufferCI, nullptr, &scratchBuffer.buffer) != VK_SUCCESS) {
        throw std::runtime_error("Could not create scratch buffer!");
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(_app._device, scratchBuffer.buffer, &memoryRequirements);

    VkMemoryAllocateFlagsInfo memoryAllocateFI{};
    memoryAllocateFI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFI.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo memoryAI{};
    memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAI.pNext = &memoryAllocateFI;
    memoryAI.allocationSize = memoryRequirements.size;

    memoryAI.memoryTypeIndex = _app.findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(_app._device, &memoryAI, nullptr, &scratchBuffer.memory) != VK_SUCCESS) {
        throw std::runtime_error("Could not allocate memory for scratch buffer!");
    }

    if (vkBindBufferMemory(_app._device, scratchBuffer.buffer, scratchBuffer.memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Could not bind buffer memory for scratch buffer!");
    }

    VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.buffer;
    scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(_app._device, &bufferDeviceAddressInfo);

    return scratchBuffer;
}

void RaytracingHandler::deleteScratchBuffer(RayTracingScratchBuffer& scratchBuffer)
{
    if (scratchBuffer.memory != VK_NULL_HANDLE) {
        vkFreeMemory(_app._device, scratchBuffer.memory, nullptr);
    }
    if (scratchBuffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_app._device, scratchBuffer.buffer, nullptr);
    }
}

void RaytracingHandler::deleteObjectMemory(RayTracingObjectMemory& objectMemory)
{
    if (objectMemory.memory != VK_NULL_HANDLE) {
        vkFreeMemory(_app._device, objectMemory.memory, nullptr);
    }
}