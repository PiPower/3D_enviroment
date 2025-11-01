#pragma once
#include <vulkan/vulkan.h>
#include <vector>

struct VulkanPipelineData
{
	VkPipeline pipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	std::vector<VkDescriptorSet> sets;
};

struct MeshCollection
{
	VkBuffer vertexBuffer;
	VkDeviceMemory vbDevMem;
	VkBuffer indexBuffer;
	VkDeviceMemory ibDevMem;
	std::vector<uint32_t> vbOffset;
	std::vector<uint32_t> ibOffset;
	std::vector<uint32_t> indexCount;
	std::vector<uint32_t> materialIndex;
};


enum class GeometryType
{
	Box,
	Mesh,
};

struct GeometryEntry
{
	GeometryType gType;
	uint32_t vertexCount;
};

