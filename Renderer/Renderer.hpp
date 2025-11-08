#pragma once
#include "VulkanResources.hpp"
#include "ShaderCompiler.hpp"
#include "GraphicsTypes.hpp"
#define VULKAN_RENDERER_ERROR 0xFFFFFFFFFFFFFFFF;
#define UBO_CAMERA_RESOURCE_TYPE 1
#define UBO_OBJ_TRSF_RESOURCE_TYPE 2

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
	int64_t UpdateUboMemory(
		uint64_t poolId,
		uint64_t uboId,
		const char* buff
	);

	int64_t CreateUboPool(
		VkDeviceSize globalUboSize,
		VkDeviceSize localUboSize, 
		VkDeviceSize poolSize,
		uint64_t* createdPoolId);

	int64_t AllocateUboResource(
		uint64_t poolId, 
		uint64_t resourceId,
		uint64_t* allocatedUboId);

	int64_t BindUboPoolToPipeline(
		uint64_t pipelineId,
		uint64_t uboPoolId,
		uint64_t cameraUboId);

	uint64_t CreateRenderItem(
		uint64_t meshCollectionId,
		uint64_t meshId,
		uint64_t uboSize = 0);

	void Render(
		uint64_t meshCollectionId,
		uint64_t pipelineId,
		const std::vector<RenderItem>& renderItems);

	void Present();
	/*
		Creates group of meshes that will be bound together to the pipeline
		Each mesh retains index from geometryEntries
		On error returns 0xFFFFFFFFFFFFFFFF
	*/
	int64_t  CreateMeshCollection(
		const std::vector<GeometryEntry>& geometryEntries,
		uint64_t* createdMeshId);

private:
	void CreateControllingStructs();
	void CreateComputePipeline();
	void CreateComputeLayout();
	void CreatePipelinePools();
	void CreateBasicGraphicsLayout();
	void CreateBasicGraphicsPipelines();
	void CreateGraphicsSets();
	void UpdateComputeSets();
public:
	uint32_t imageIndex;
	VkDeviceSize maxUboPoolSize;
	VulkanResources vkResources;
	HWND windowHwnd;
	VkSubmitInfo submitInfo;
	ShaderCompiler compiler;
	VkImageCopy computeSwapchainCopy;
	AllocatedBuffer stagingBuffer;
	char* stagingPtr;
	std::vector<VkRenderPassBeginInfo> renderPassInfos;
	std::vector<VkImageMemoryBarrier> transferBarriers;
	std::vector<VkImageMemoryBarrier> restoreBarriers;
	std::vector<MeshCollection> meshCollections;
	std::vector<VulkanPipelineData> pipelines;
	std::vector<UboPoolEntry> uboPoolEntries;
};