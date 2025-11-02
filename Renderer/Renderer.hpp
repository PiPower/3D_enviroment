#pragma once
#include "VulkanResources.hpp"
#include "ShaderCompiler.hpp"
#include "GraphicsTypes.hpp"

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
		VkDeviceSize uboPoolSize = 1'000'000,
		VkDeviceSize stagingSize = 1'000'000);
	void BeginRendering();
	void UpdateCamera(const Camera* ptr);
	uint64_t CreateRenderItem(uint64_t size);
	void Render(uint64_t meshCollectionIdx, uint64_t pipelineIdx, std::vector<uint64_t> renderItems);
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
};