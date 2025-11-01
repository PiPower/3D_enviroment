#pragma once


struct DepthBufferBundle
{
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
};

static VkInstance createInstance();

static bool checkSupportForExt(
	const char** requiredExtensions,
	uint32_t requiredExtensionsNum,
	const VkExtensionProperties* supportedExtensions,
	uint32_t supportedExtensionsNum);

static VkDebugUtilsMessengerEXT setupDebug(
	VkInstance instance);

static VkBool32 vbDebugVal(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

static VkSurfaceKHR createSurface(
	VkInstance instance,
	HINSTANCE hinstance,
	HWND hwnd);

static QueuesIdx createQueueIndecies(
	VkInstance instance,
	VkPhysicalDevice dev,
	VkSurfaceKHR surface);

static VkDevice createLogicalDevice(
	VkPhysicalDevice physicalDev,
	QueuesIdx queuesIdx);

static VkPhysicalDevice pickPhysicalDevice(
	VkInstance instance);

static SwapchainInfo querySwapChainSupport(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface);

static VkSwapchainKHR createSwapchain(
	VkDevice device,
	VkSurfaceKHR surface,
	const SwapchainInfo& swapchainInfo,
	const QueuesIdx& queuesIdx,
	VkFormat* chosenFormat);

static Texture createTexture2D(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageUsageFlags usage);

static VkDeviceMemory allocateBuffer_temp(
	VkDevice device, 
	VkPhysicalDevice physicalDevice,
	VkMemoryRequirements memRequirements,
	VkMemoryPropertyFlags properties);

static std::vector<VkImage> createSwapchainImages(
	VkDevice device,
	VkSwapchainKHR swapchain);

static std::vector<VkImageView> createSwapchainImageViews(
	VkDevice device,
	const std::vector<VkImage>& images,
	const SwapchainInfo& swcInfo,
	const VkFormat&	imgFormat);

static VkCommandPool createCommandPool(
	VkDevice device,
	uint32_t queueIdx);

static std::vector<VkCommandBuffer> createCommandBuffers(
	VkDevice device,
	VkCommandPool cmdPool,
	uint32_t bufferCount);

static VkRenderPass createRenderPass(
	VkDevice device,
	VkFormat imgFormat);

static DepthBufferBundle createDepthBuffer(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	const SwapchainInfo& swcInfo);

static std::vector<VkFramebuffer> createFramebuffers(
	VkDevice device,
	VkRenderPass renderPass,
	VkImageView	depthView,
	const std::vector<VkImageView> imgViews,
	const SwapchainInfo& swcInfo);
