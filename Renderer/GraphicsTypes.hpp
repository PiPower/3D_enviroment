#pragma once
#include "VulkanResources.hpp"
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
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	std::vector<uint32_t> vbOffset;
	std::vector<uint32_t> ibOffset;
	std::vector<uint32_t> indexCount;
};


enum class GeometryType
{
	//built in meshes
	Box,
	//mesh is user provided mesh
	Mesh,
};

/*
	When vertexBuffer, indexBuffer, vertexCount, indexCount are not used when gType == built-in type.
*/
struct GeometryEntry
{
	GeometryEntry(GeometryType type)
	: gType(type), vertexBuffer(nullptr), indexBuffer(nullptr),
	  vertexSize(0), vertexCount(0), indexCount(0)
	{}

	GeometryType gType;
	char* vertexBuffer;
	char* indexBuffer;
	uint16_t vertexSize;
	uint32_t vertexCount;
	uint32_t indexCount;
};

