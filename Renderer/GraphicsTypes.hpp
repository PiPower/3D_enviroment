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
	//built in meshes
	Box,
	//mesh is user provided mesh
	Mesh,
};

/*
	When vertexBuffer and indexBuffer are not used when gType == built-in type.
*/
struct GeometryEntry
{
	GeometryType gType;
	char* vertexBuffer;
	char* indexBuffer;
};

