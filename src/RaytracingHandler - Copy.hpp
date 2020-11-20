#include "Application.hpp"
#include "RaytracingHandler.hpp"

RaytracingHandler::RaytracingHandler(Application& app)
    : _app(app)
{
    _rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &_rtProperties;
    vkGetPhysicalDeviceProperties2(app._physDevice, &deviceProps2);

    _rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &_rtFeatures;
    vkGetPhysicalDeviceFeatures2(app._physDevice, &deviceFeatures2);

    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(app._device, "vkGetBufferDeviceAddressKHR"));
    vkBindAccelerationStructureMemoryKHR = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryKHR>(vkGetDeviceProcAddr(app._device, "vkBindAccelerationStructureMemoryKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(app._device, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(app._device, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureMemoryRequirementsKHR = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsKHR>(vkGetDeviceProcAddr(app._device, "vkGetAccelerationStructureMemoryRequirementsKHR"));
    vkCmdBuildAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureKHR>(vkGetDeviceProcAddr(app._device, "vkCmdBuildAccelerationStructureKHR"));
    vkBuildAccelerationStructureKHR = reinterpret_cast<PFN_vkBuildAccelerationStructureKHR>(vkGetDeviceProcAddr(app._device, "vkBuildAccelerationStructureKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(app._device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(app._device, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(app._device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(app._device, "vkCreateRayTracingPipelinesKHR"));

    createBottomLevelAccelerationStructure();
    createTopLevelAccelerationStructure();
}

RaytracingHandler::~RaytracingHandler()
{
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
    accelerationCreateGeometryInfo.maxPrimitiveCount = _app._models[0].getNumberOfPrimitives();
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
    VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(device, &accelerationCI, nullptr, &bottomLevelAS.accelerationStructure));

}

void RaytracingHandler::createTopLevelAccelerationStructure()
{
}
