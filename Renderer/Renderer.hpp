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
	ShaderCompiler compiler;
	VkDescriptorPool pipelinesPool;
	VkImageCopy computeSwapchainCopy;
	std::vector<VkRenderPassBeginInfo> renderPassInfos;
	std::vector<VkImageMemoryBarrier> transferBarriers;
	std::vector<VkImageMemoryBarrier> restoreBarriers;
	std::vector<VulkanPipelineData> pipelines;
};