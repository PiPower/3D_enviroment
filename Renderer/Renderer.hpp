#pragma once
#include "VulkanResources.hpp"
#include "ShaderCompiler.hpp"
#include "GraphicsTypes.hpp"
#define VULKAN_RENDERER_ERROR 0xFFFFFFFFFFFFFFFF;
#define UBO_GLOBAL_RESOURCE_TYPE 0
#define UBO_OBJ_TRSF_RESOURCE_TYPE 1

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
		TextureDim* skyboxDim,
		VkDeviceSize stagingSize = 1'000'000);

	int64_t CreateGraphicsPipeline(
		uint8_t lightCount,
		const std::vector<TextureDim>& textureDims,
		uint64_t* pipeline,
		bool isShadowPipeline = false);

	int64_t UploadTexture(
		uint64_t pipelineId,
		uint8_t textureId,
		const char* data,
		uint32_t texInstanceIdx = 0);

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
		uint64_t globalUboId);


	int64_t UpdateSkyboxData(
		const std::vector<const char*>& facePtrArray,
		uint64_t uboPoolId,
		uint64_t globalUboId);

	void BeginShadowPass();

	void BeginRenderPass();

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
	int64_t CreateMeshCollection(
		const std::vector<GeometryEntry>& geometryEntries,
		uint64_t* createdMeshId);

private:
	void CreateControllingStructs();

	void CreateComputePipeline(
		VkPipelineLayout pipelineLayout,
		VkPipeline* pipeline);

	void CreateComputeLayout(
		VkDescriptorSetLayout* setLayout,
		VkPipelineLayout* pipelineLayout);

	void CreateBasicGraphicsLayout(
		uint8_t textureCount,
		VkDescriptorSetLayout* setLayout,
		VkPipelineLayout* pipelineLayout);

	void CreateBasicGraphicsVkPipeline(
		VkPipelineLayout pipelineLayout,
		VkPipeline* pipeline,
		uint8_t lightCount,
		uint8_t textureCount,
		bool isShadowPipeline);

	void CreateGraphicsSets(
		VulkanPipelineData* pipelineData);

	void InitComputeSets(
		VulkanPipelineData* pipelineData);

	void SetImageSets(
		VulkanPipelineData* pipelineData);

	void CreateSampler(
		VulkanPipelineData* pipelineData);

	void PrepareTextureData(
		VulkanPipelineData* pipelineData,
		const std::vector<TextureDim>& texDims,
		bool isCubemap = false);

	void CreateSkyboxPipeline(
		TextureDim* skyboxDim);

private:
	uint32_t imageIndex;
	VkDeviceSize maxUboPoolSize;
	VulkanResources vkResources;
	HWND windowHwnd;
	VkSubmitInfo submitInfo;
	ShaderCompiler compiler;
	VkImageCopy computeSwapchainCopy;
	AllocatedBuffer stagingBuffer;
	uint64_t skyboxPipelineId;
	char* stagingPtr;
	std::vector<VkRenderPassBeginInfo> renderPassInfos;
	std::vector<VkImageMemoryBarrier> transferBarriers;
	std::vector<VkImageMemoryBarrier> restoreBarriers;
	std::vector<MeshCollection> meshCollections;
	std::vector<VulkanPipelineData> pipelines;
	std::vector<UboPoolEntry> uboPoolEntries;
};