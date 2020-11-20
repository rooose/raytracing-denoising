#pragma once

#include <vulkan/vulkan_beta.h>

class Application;

struct RayTracingObjectMemory
{
	uint64_t deviceAddress = 0;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct AccelerationStructure {
	VkAccelerationStructureKHR accelerationStructure;
	uint64_t handle;
	RayTracingObjectMemory objectMemory;
};

struct RayTracingScratchBuffer
{
	uint64_t deviceAddress = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

class RaytracingHandler {
public:
	RaytracingHandler(Application& app);
	~RaytracingHandler();

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkBindAccelerationStructureMemoryKHR vkBindAccelerationStructureMemoryKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureMemoryRequirementsKHR vkGetAccelerationStructureMemoryRequirementsKHR;
	PFN_vkCmdBuildAccelerationStructureKHR vkCmdBuildAccelerationStructureKHR;
	PFN_vkBuildAccelerationStructureKHR vkBuildAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

private:
	Application& _app;
	VkPhysicalDeviceRayTracingPropertiesKHR  _rtProperties;
	VkPhysicalDeviceRayTracingFeaturesKHR _rtFeatures;
	
	AccelerationStructure bottomLevelAS;
	AccelerationStructure topLevelAS;

	void createBottomLevelAccelerationStructure();
	void createTopLevelAccelerationStructure();

};
