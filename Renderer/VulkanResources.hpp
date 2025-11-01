#pragma once
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <inttypes.h>

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

struct VulkanResources
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;
	QueuesIdx queueFamilies;
	VkQueue graphicsQueue;
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
