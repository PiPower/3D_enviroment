#pragma once
#include "VulkanResources.hpp"
#include "ShaderCompiler.hpp"
#include "GraphicsTypes.hpp"
#define VULKAN_RENDERER_ERROR 0xFFFFFFFFFFFFFFFF;

enum class PipelineTypes
{
	Compute = 0,
	Graphics,
	GraphicsNonFill
};


class Renderer
{
public:
	Renderer(
		HINSTANCE hinstance, 
		HWND hwnd,
		VkDeviceSize stagingSize = 1'000'000);

	void BeginRendering();
	/*
		maximum number of render items per mesh collection is 0xFFFFFFFFFFFF
	*/
	uint64_t AllocateUboPool(VkDeviceSize globalUboSize, VkDeviceSize localUboSize, VkDeviceSize poolSize = 0);
	uint64_t CreateRenderItem(uint64_t meshCollectionId, uint64_t meshId, uint64_t uboSize = 0);
	void Render(uint64_t meshCollectionId, uint64_t pipelineId, std::vector<uint64_t> renderItems);
	void Present();
	/*
		Creates group of meshes that will be bound together to the pipeline
		Each mesh retains index from geometryEntries
		On error returns 0xFFFFFFFFFFFFFFFF
	*/
	uint64_t CreateMeshCollection(const std::vector<GeometryEntry>& geometryEntries);
private:
	void CreateControllingStructs();
	void CreateComputePipeline();
	void CreateComputeLayout();
	void CreateDescriptorSets();
	void CreateBasicGraphicsLayout();
	void CreateBasicGraphicsPipelines();
public:
	uint32_t imageIndex;
	VkDeviceSize uboPoolSize;
	VulkanResources vkResources;
	HWND windowHwnd;
	VkSubmitInfo submitInfo;
	ShaderCompiler compiler;
	VkDescriptorPool pipelinesPool;
	VkImageCopy computeSwapchainCopy;
	AllocatedBuffer stagingBuffer;
	char* stagingPtr;
	std::vector<VkRenderPassBeginInfo> renderPassInfos;
	std::vector<VkImageMemoryBarrier> transferBarriers;
	std::vector<VkImageMemoryBarrier> restoreBarriers;
	std::vector<MeshCollection> meshCollections;
	std::vector<VulkanPipelineData> pipelines;
	std::vector<MemoryPool> uboPools;
};