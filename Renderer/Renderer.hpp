#pragma once
#include "VulkanResources.hpp"
#include "ShaderCompiler.hpp"

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


class Renderer
{
public:
	Renderer(
		HINSTANCE hinstance, 
		HWND hwnd);
	void BeginRendering();
	void Render();
	void Present();
	void CreateMeshCollection();
private:
	void SetControllingStructs();
	void SetupComputePipeline();
	void SetupComputeLayout();
	void SetupDescriptorSets();
public:
	uint32_t imageIndex;
	VulkanResources vkResources;
	HWND windowHwnd;
	VkSubmitInfo submitInfo;
	std::vector<VkRenderPassBeginInfo> renderPassInfos;
	std::vector<VkImageMemoryBarrier> transferBarriers;
	std::vector<VkImageMemoryBarrier> restoreBarriers;
	ShaderCompiler compiler;
	VulkanPipelineData compute;
	VkDescriptorPool pipelinesPool;
	VkImageCopy computeSwapchainCopy;
};