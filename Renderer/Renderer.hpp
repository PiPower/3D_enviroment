#pragma once
#include "VulkanResources.hpp"
#include "ShaderCompiler.hpp"
#include "GraphicsTypes.hpp"

class Renderer
{
public:
	Renderer(
		HINSTANCE hinstance, 
		HWND hwnd);
	void BeginRendering();
	void Render();
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
	std::vector<VkRenderPassBeginInfo> renderPassInfos;
	std::vector<VkImageMemoryBarrier> transferBarriers;
	std::vector<VkImageMemoryBarrier> restoreBarriers;
	std::vector<MeshCollection> meshCollections;
	std::vector<VulkanPipelineData> pipelines;
};