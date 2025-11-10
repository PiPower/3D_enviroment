#pragma once
#include "VulkanResources.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <DirectXMath.h>

struct VulkanPipelineData
{
	VkPipeline pipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	std::vector<VkDescriptorSet> sets;
	VkDescriptorPool descriptorPool;
	size_t boundPoolId;
	size_t boundCameraUboId;
	uint32_t maxSets;
};

struct RenderItem
{
	uint32_t vertexOffset;
	uint32_t indexOffset;
	uint64_t transformUboId;
	uint32_t indexCountOffset;
};

struct MeshCollection
{
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	std::vector<uint32_t> vbOffset;
	std::vector<uint32_t> ibOffset;
	std::vector<uint32_t> indexCount;
	std::vector<RenderItem> items;
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

struct Camera 
{
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;
};

struct ObjectTransform
{
	DirectX::XMFLOAT4X4 transform;
	float color[4];
};

struct UboEntry
{
	VkDeviceSize bufferOffset;
	uint64_t resourceId;
	size_t bufferIdx;
	// bit 0: is alive
	int32_t traits;
};

/*
	Continous sequence of ubo buffers in memory
*/
struct UboPoolEntry
{
	std::vector<UboEntry> uboEntries;
	MemoryPool uboPool;
	VkDeviceSize bufferOffset;
	size_t bufferIdx;
	char* memoryMap;
};

inline char IS_UBO_ENTRY_ALIVE(const UboEntry* ubo) { return ubo->traits & 0x01; }
inline void SET_UBO_ENTRY_ALIVE(UboEntry* ubo) { ubo->traits = ubo->traits | 0x01; }
inline void SET_UBO_ENTRY_DEAD(UboEntry* ubo) { ubo->traits = ubo->traits & ~0x01; }