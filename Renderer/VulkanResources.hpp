#pragma once
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <inttypes.h>
#define VK_RESOURCES_OK 0


struct QueuesIdx
{
	int64_t grahicsIdx;
	int64_t presentationIdx;
};

struct SwapchainInfo
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Texture
{
	VkImage texImage;
	VkDeviceMemory memory;
	VkImageView texView;
	VkDeviceSize alignment;
};

struct AllocatedBuffer
{
	VkBuffer buffer;
	VkMemoryRequirements requirements;
	VkDeviceMemory deviceMemory;
};

struct MemoryPool
{
	VkDeviceMemory deviceMemory;
	std::vector<VkBuffer> boundBuffers;
	std::vector<VkBufferCreateInfo> bufferInfos;
	std::vector<VkMemoryRequirements> resourceReqs;
	VkDeviceSize poolSize;
	VkDeviceSize maxAlignment;
	VkDeviceSize currOffset;
};

struct CpuBuffer
{
	const char* data;
	VkDeviceSize size;
};

struct VulkanResources
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;
	QueuesIdx queueFamilies;
	VkQueue graphicsQueue;
	VkQueue sideQueue;
	VkQueue presentationQueue;
	VkDebugUtilsMessengerEXT debug;
	SwapchainInfo swapchainInfo;
	VkFormat swapchainFormat;
	VkSwapchainKHR swapchain;
	std::vector<Texture> renderTextures;
	std::vector<Texture> computeTextures;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkCommandPool cmdPool;
	VkCommandBuffer sideCmdBuffer;
	VkCommandBuffer cmdBuffer;
	VkRenderPass renderPass;
	VkSemaphore imgReady;
	VkSemaphore renderingFinished;
	VkFence gfxQueueFinished;
};

int64_t createVulkanResources(
	VulkanResources* vkResources, 
	HINSTANCE hinstance, 
	HWND hwnd);

VkResult allocateBuffer(
	VkDevice device, 
	VkPhysicalDevice physicalDevice,
	VkDeviceSize buffSize, 
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProps,
	AllocatedBuffer* allocatedBuffer );

VkResult uploadStagingData(
	VkCommandBuffer cmdBuffer,
	VkQueue executionQueue,
	VkBuffer stagingBuffer,
	char* stagingBufferPtr,
	VkDeviceSize stagingSize,
	VkBuffer destinationBuffer,
	const std::vector<CpuBuffer>& buffers);

VkResult performBufferCopy(
	VkCommandBuffer cmdBuffer,
	VkQueue executionQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size,
	VkDeviceSize srcOffset = 0,
	VkDeviceSize dstOffset = 0);

VkResult allocateMemoryPool(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize poolSize,
	const std::vector<VkDeviceSize>& bufferSizes,
	const std::vector<VkBufferUsageFlags>& flags,
	VkMemoryPropertyFlags usageFlags,
	MemoryPool* pool);

VkResult findOffsetInBuffer(
	VkDeviceSize currOffsetInBuffer,
	VkDeviceSize requiredAlignment,
	VkDeviceSize allocationSize,
	VkDeviceSize bufferSize,
	VkDeviceSize resourceSize,
	VkDeviceSize* resourceOffset,
	VkDeviceSize* memoryUpdateSize);

VkResult createBufferInPool(
	size_t resourceIdx,
	VkDevice device,
	MemoryPool* pool
);

VkResult getBufferMemoryRequirements(
	VkDevice device,
	VkDeviceSize size,
	VkBufferUsageFlags usageFlags,
	VkMemoryRequirements* reqs,
	VkBuffer* buffer = nullptr
);