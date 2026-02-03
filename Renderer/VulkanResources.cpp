#include "VulkanResources.hpp"
#include "VulkanResourcesInternal.hpp"
#include "errors.hpp"
#pragma comment(lib,"C:\\VulkanSDK\\1.4.304.1\\Lib\\vulkan-1.lib")
#define RETURN_ON_VK_ERROR(expr){VkResult __result__ = (expr); if(__result__ != VK_SUCCESS){return __result__;}}
#define JUMP_TO_ON_VK_ERROR(expr, label){VkResult __result__ = (expr); if(__result__ != VK_SUCCESS){goto label;}}
using namespace std;
const static char* instExt[] = {
					 VK_KHR_SURFACE_EXTENSION_NAME,
					 VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
					 VK_KHR_WIN32_SURFACE_EXTENSION_NAME };


const static char* devExt[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const static char* vaLayers[] = { "VK_LAYER_KHRONOS_validation" };

int64_t createVulkanResources(
	VulkanResources* vkResources,
	HINSTANCE hinstance,
	HWND hwnd)
{
	vkResources->instance = createInstance();
#ifdef _DEBUG
	vkResources->debug = setupDebug(vkResources->instance);
#endif 
	vkResources->physicalDevice = pickPhysicalDevice(vkResources->instance);
	vkResources->surface = createSurface(vkResources->instance, hinstance, hwnd);
	vkResources->queueFamilies = createQueueIndecies(vkResources->instance, vkResources->physicalDevice, vkResources->surface);
	vkResources->device = createLogicalDevice(vkResources->physicalDevice, vkResources->queueFamilies);
	vkGetDeviceQueue(vkResources->device, vkResources->queueFamilies.grahicsIdx, 0, &vkResources->graphicsQueue);
	vkGetDeviceQueue(vkResources->device, vkResources->queueFamilies.grahicsIdx, 1, &vkResources->sideQueue);
	vkGetDeviceQueue(vkResources->device, vkResources->queueFamilies.presentationIdx, 0, &vkResources->presentationQueue);
	vkResources->swapchainInfo = querySwapChainSupport(vkResources->physicalDevice, vkResources->surface);
	vkResources->swapchain = createSwapchain(vkResources->device, vkResources->surface, vkResources->swapchainInfo,
															vkResources->queueFamilies, &vkResources->swapchainFormat);
	vkResources->swapchainImages = createSwapchainImages(vkResources->device, vkResources->swapchain);
	vkResources->swapchainImageViews = createSwapchainImageViews(vkResources->device, vkResources->swapchainImages,
															vkResources->swapchainInfo, vkResources->swapchainFormat);
	vkResources->renderTextures.resize(vkResources->swapchainImages.size());
	vkResources->computeTextures.resize(vkResources->swapchainImages.size());
	vector<VkImageView> textureViews(vkResources->swapchainImages.size());
	for (size_t i = 0; i < vkResources->swapchainImages.size(); i++)
	{
		vkResources->renderTextures[i] = createTexture2D(vkResources->device, vkResources->physicalDevice,
			vkResources->swapchainInfo.capabilities.currentExtent.width,
			vkResources->swapchainInfo.capabilities.currentExtent.height,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		vkResources->computeTextures[i] = createTexture2D(vkResources->device, vkResources->physicalDevice,
			vkResources->swapchainInfo.capabilities.currentExtent.width,
			vkResources->swapchainInfo.capabilities.currentExtent.height,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		textureViews[i] = vkResources->renderTextures[i].texView;
	}

	vkResources->cmdPool = createCommandPool(vkResources->device, vkResources->queueFamilies.grahicsIdx);
	vector<VkCommandBuffer> cmdBuffs = createCommandBuffers(vkResources->device, vkResources->cmdPool, 2);
	vkResources->cmdBuffer = cmdBuffs[0];
	vkResources->sideCmdBuffer = cmdBuffs[1];
	vkResources->renderPass = createRenderPass(vkResources->device, VK_FORMAT_R8G8B8A8_UNORM);
	DepthBufferBundle bundle = createDepthBuffer(vkResources->device, vkResources->physicalDevice, 
			vkResources->swapchainInfo.capabilities.currentExtent.width, vkResources->swapchainInfo.capabilities.currentExtent.height);
	vkResources->depthImage = bundle.depthImage;
	vkResources->depthImageMemory = bundle.depthImageMemory;
	vkResources->depthImageView = bundle.depthImageView;
	vkResources->shadowmapTexture = createDepthBuffer(vkResources->device, vkResources->physicalDevice,
		vkResources->swapchainInfo.capabilities.currentExtent.width, vkResources->swapchainInfo.capabilities.currentExtent.height, true);

	vkResources->swapchainFramebuffers = createFramebuffers(vkResources->device, vkResources->renderPass,
		vkResources->depthImageView, vkResources->shadowmapTexture.depthImageView, textureViews, vkResources->swapchainInfo);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	EXIT_ON_VK_ERROR(vkCreateSemaphore(vkResources->device, &semaphoreInfo, nullptr, &vkResources->imgReady));
	EXIT_ON_VK_ERROR(vkCreateSemaphore(vkResources->device, &semaphoreInfo, nullptr, &vkResources->renderingFinished));

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	EXIT_ON_VK_ERROR(vkCreateFence(vkResources->device, &fenceInfo, nullptr, &vkResources->gfxQueueFinished));

	// transition all required resources 
	VkCommandBufferBeginInfo cmdBuffInfo = {};
	cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	EXIT_ON_VK_ERROR(vkResetCommandBuffer(vkResources->cmdBuffer, 0));
	EXIT_ON_VK_ERROR(vkBeginCommandBuffer(vkResources->cmdBuffer, &cmdBuffInfo));


	VkImageMemoryBarrier depthBarrier = {};
	depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	depthBarrier.srcAccessMask = 0;
	depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	depthBarrier.image = vkResources->depthImage;
	depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthBarrier.subresourceRange.baseMipLevel = 0;
	depthBarrier.subresourceRange.levelCount = 1;
	depthBarrier.subresourceRange.baseArrayLayer = 0;
	depthBarrier.subresourceRange.layerCount = 1;

	vector<VkImageMemoryBarrier> barriers;
	barriers.resize(1 + vkResources->swapchainImages.size() * 2);
	barriers[vkResources->swapchainImages.size() * 2] = depthBarrier;
	for (size_t i = 0; i < vkResources->swapchainImages.size(); i++)
	{
		barriers[2 * i] = {};
		barriers[2 * i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[2 * i].srcAccessMask = 0;
		barriers[2 * i].dstAccessMask = 0;
		barriers[2 * i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[2 * i].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barriers[2 * i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[2 * i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[2 * i].image = vkResources->swapchainImages[i];
		barriers[2 * i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barriers[2 * i].subresourceRange.baseMipLevel = 0;
		barriers[2 * i].subresourceRange.levelCount = 1;
		barriers[2 * i].subresourceRange.baseArrayLayer = 0;
		barriers[2 * i].subresourceRange.layerCount = 1;

		barriers[2 * i + 1] = {};
		barriers[2 * i + 1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[2 * i + 1].srcAccessMask = 0;
		barriers[2 * i + 1].dstAccessMask = 0;
		barriers[2 * i + 1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[2 * i + 1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barriers[2 * i + 1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[2 * i + 1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[2 * i + 1].image = vkResources->computeTextures[i].texImage;
		barriers[2 * i + 1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barriers[2 * i + 1].subresourceRange.baseMipLevel = 0;
		barriers[2 * i + 1].subresourceRange.levelCount = 1;
		barriers[2 * i + 1].subresourceRange.baseArrayLayer = 0;
		barriers[2 * i + 1].subresourceRange.layerCount = 1;
	}

	vkCmdPipelineBarrier(vkResources->cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());

	EXIT_ON_VK_ERROR(vkEndCommandBuffer(vkResources->cmdBuffer));
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkResources->cmdBuffer;
	EXIT_ON_VK_ERROR(vkQueueSubmit(vkResources->graphicsQueue, 1, &submitInfo, nullptr));
	EXIT_ON_VK_ERROR(vkQueueWaitIdle(vkResources->graphicsQueue));

	return 0;
}

VkResult allocateBuffer(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize buffSize,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProps,
	AllocatedBuffer* allocatedBuffer)
{
	VkResult result;
	VkBufferCreateInfo buffInfo = {};
	buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffInfo.size = buffSize;
	buffInfo.usage = usage;
	buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	RETURN_ON_VK_ERROR(vkCreateBuffer(device, &buffInfo, nullptr, &allocatedBuffer->buffer));

	vkGetBufferMemoryRequirements(device, allocatedBuffer->buffer, &allocatedBuffer->requirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	uint32_t i;
	for (i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((allocatedBuffer->requirements.memoryTypeBits & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & memoryProps) == memoryProps) {
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = allocatedBuffer->requirements.size;
	allocInfo.memoryTypeIndex = i;

	RETURN_ON_VK_ERROR(vkAllocateMemory(device, &allocInfo, nullptr, &allocatedBuffer->deviceMemory));
	RETURN_ON_VK_ERROR(vkBindBufferMemory(device, allocatedBuffer->buffer, allocatedBuffer->deviceMemory, 0));

	return VK_SUCCESS;
}


VkResult uploadStagingData(
	VkCommandBuffer cmdBuffer, 
	VkQueue executionQueue, 
	VkBuffer stagingBuffer,
	char* stagingBufferPtr,
	VkDeviceSize stagingSize, 
	VkBuffer destinationBuffer,
	const std::vector<CpuBuffer>& buffers)
{

	VkDeviceSize currentBufferSize = 0;
	VkDeviceSize uploadedData = 0;
	for (size_t i = 0; i < buffers.size(); i++)
	{
		const CpuBuffer* buffer = &buffers[i];
		VkDeviceSize inBufferOffset = 0;
		while (buffer->size - inBufferOffset > 0)
		{
			if (currentBufferSize == stagingSize)
			{
				RETURN_ON_VK_ERROR(performBufferCopy(cmdBuffer, executionQueue, stagingBuffer, destinationBuffer, currentBufferSize, 0, uploadedData));
				uploadedData += currentBufferSize;
				currentBufferSize = 0;
			}


			VkDeviceSize uploadSize = min(buffer->size - inBufferOffset, stagingSize - currentBufferSize);
			memcpy(stagingBufferPtr + currentBufferSize, buffer->data + inBufferOffset, uploadSize);
			currentBufferSize += uploadSize;
			inBufferOffset += uploadSize;
		}

	}

	if (currentBufferSize > 0)
	{
		RETURN_ON_VK_ERROR(performBufferCopy(cmdBuffer, executionQueue, stagingBuffer, destinationBuffer, currentBufferSize, 0, uploadedData));
		uploadedData += currentBufferSize;
		currentBufferSize = 0;
	}

	return VK_SUCCESS;
}

VkResult performBufferCopy(
	VkCommandBuffer cmdBuffer,
	VkQueue executionQueue,
	VkBuffer srcBuffer, 
	VkBuffer dstBuffer, 
	VkDeviceSize size,
	VkDeviceSize srcOffset,
	VkDeviceSize dstOffset)
{
	VkBufferCopy copy;
	copy.size = size;
	copy.srcOffset = srcOffset;
	copy.dstOffset = dstOffset;
	// transition all required resources 
	VkCommandBufferBeginInfo cmdBuffInfo = {};
	cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	RETURN_ON_VK_ERROR(vkResetCommandBuffer(cmdBuffer, 0));
	RETURN_ON_VK_ERROR(vkBeginCommandBuffer(cmdBuffer, &cmdBuffInfo));

	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copy);

	RETURN_ON_VK_ERROR(vkEndCommandBuffer(cmdBuffer));
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	RETURN_ON_VK_ERROR(vkQueueSubmit(executionQueue, 1, &submitInfo, nullptr));
	RETURN_ON_VK_ERROR(vkQueueWaitIdle(executionQueue));

}

VkResult allocateMemoryPool(
	VkDevice device, 
	VkPhysicalDevice physicalDevice,
	VkDeviceSize poolSize, 
	const std::vector<VkDeviceSize>& bufferSizes, 
	const std::vector<VkBufferUsageFlags>& usageFlags,
	VkMemoryPropertyFlags memProps,
	MemoryPool* pool)
{
	VkMemoryAllocateInfo allocInfo = {};
	uint32_t heapIdx = 0;
	bool heapSucces = false;

	pool->currOffset = 0;
	pool->maxAlignment = 0;
	pool->poolSize = poolSize;
	pool->resourceReqs.resize(bufferSizes.size());
	pool->bufferInfos.resize(bufferSizes.size());

	VkResult result = VK_SUCCESS;
	VkPhysicalDeviceMemoryProperties properties;

	for (size_t i = 0; i < bufferSizes.size(); i++)
	{
		pool->bufferInfos[i] = {};
		pool->bufferInfos[i].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		pool->bufferInfos[i].size = bufferSizes[i];
		pool->bufferInfos[i].usage = usageFlags[i];
		pool->bufferInfos[i].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		result = getBufferMemoryRequirements(device, bufferSizes[i], usageFlags[i], &pool->resourceReqs[i]);
		pool->maxAlignment = max(pool->resourceReqs[i].alignment, pool->maxAlignment);
		if (result != VK_SUCCESS)
		{
			goto clean;
		};
		if (pool->resourceReqs[i].size > poolSize)
		{
			result = VK_ERROR_OUT_OF_POOL_MEMORY;
			goto clean;
		}

	}

	if (bufferSizes.size() == 0)
	{
		pool->resourceReqs.resize(1);
		pool->bufferInfos.resize(1);

		pool->bufferInfos[0] = {};
		pool->bufferInfos[0].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		pool->bufferInfos[0].size = poolSize;
		pool->bufferInfos[0].usage = usageFlags[0];
		pool->bufferInfos[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		result = getBufferMemoryRequirements(device, poolSize, usageFlags[0], &pool->resourceReqs[0]);
		pool->maxAlignment = max(pool->resourceReqs[0].alignment, pool->maxAlignment);
		if (result != VK_SUCCESS)
		{
			goto clean;
		};
		if (pool->resourceReqs[0].size >= poolSize)
		{
			result = VK_ERROR_OUT_OF_POOL_MEMORY;
			goto clean;
		}
	}

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
	for (heapIdx = 0; heapIdx < properties.memoryTypeCount; heapIdx++)
	{
		
		if ((properties.memoryTypes[heapIdx].propertyFlags & memProps) == memProps)
		{
			size_t buffIdx = 0;
			while (buffIdx < pool->resourceReqs.size() &&
				   pool->resourceReqs[buffIdx].memoryTypeBits & (1 << heapIdx))
			{
				buffIdx++;
			}
			if (buffIdx == pool->resourceReqs.size())
			{
				heapSucces = true;
				break;
			}
		}
	}

	if (!heapSucces)
	{
		result = VK_ERROR_UNKNOWN;
		goto clean;
	}

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = poolSize;
	allocInfo.memoryTypeIndex = heapIdx;
	result = vkAllocateMemory(device, &allocInfo, nullptr, &pool->deviceMemory);
	if (result != VK_SUCCESS)
	{
		goto clean;
	};

	return VK_SUCCESS;

clean:

	if (pool->deviceMemory)
	{
		vkFreeMemory(device, pool->deviceMemory, nullptr);
	}
	return result;
}

VkResult findOffsetInBuffer(
	VkDeviceSize currOffsetInBuffer,
	VkDeviceSize requiredAlignment,
	VkDeviceSize allocationSize,
	VkDeviceSize bufferSize,
	VkDeviceSize resourceSize,
	VkDeviceSize* resourceOffset,
	VkDeviceSize* memoryUpdateSize)
{
	VkDeviceSize currOffset = currOffsetInBuffer;
	VkDeviceSize memoryAlignOffset = currOffset % requiredAlignment;
	if (memoryAlignOffset != 0)
	{
		if (currOffset + memoryAlignOffset >= bufferSize)
		{
			return VK_ERROR_OUT_OF_POOL_MEMORY;
		}
		currOffset += memoryAlignOffset;
	}

	if (bufferSize - currOffset < resourceSize)
	{
		return VK_ERROR_TOO_MANY_OBJECTS;
	}

	*resourceOffset = currOffset;
	*memoryUpdateSize = allocationSize + memoryAlignOffset;

	return VK_SUCCESS;
}

VkResult createBufferInPool(
	size_t resourceIdx,
	VkDevice device,
	MemoryPool* pool)
{
	VkDeviceSize resourceOffset, memoryUpdateSize;
	const VkMemoryRequirements& resourceReq = pool->resourceReqs[resourceIdx];
	const VkBufferCreateInfo resourceInfo = pool->bufferInfos[resourceIdx];
	VkBuffer buffer = nullptr;
	VkResult result;

	JUMP_TO_ON_VK_ERROR(result =  findOffsetInBuffer(pool->currOffset, resourceReq.alignment, resourceReq.size,
				pool->poolSize, resourceInfo.size, &resourceOffset, &memoryUpdateSize), clean) ;
	JUMP_TO_ON_VK_ERROR(result = vkCreateBuffer(device, &pool->bufferInfos[resourceIdx], nullptr, &buffer), clean);
	JUMP_TO_ON_VK_ERROR(result = vkBindBufferMemory(device, buffer, pool->deviceMemory, resourceOffset), clean);
	/*
		TO BE CHECKED LATER
		possible bug
		spec states that requirements has number of bytes required to allocate for the resource,
		yet it does not mention wheter this many bytes should be bound to resource. If there is object 1 with size 123 
		and alignment 256 and second object with size 100 and alignment 128 there MIGHT be possibility
		of resources boundary crossing.
	*/ 
	pool->currOffset += memoryUpdateSize;
	pool->boundBuffers.push_back(buffer);
	return VK_SUCCESS;
clean:
	if (buffer)
	{
		vkDestroyBuffer(device, buffer, nullptr);
	}
	return result;
}

VkResult getBufferMemoryRequirements(
	VkDevice device,
	VkDeviceSize size,
	VkBufferUsageFlags usageFlags,
	VkMemoryRequirements* reqs,
	VkBuffer* buffer)
{
	VkBuffer tmpBuff;
	VkBuffer* ptrBuff;
	if (!buffer)
	{
		ptrBuff = &tmpBuff;
	}
	else
	{
		ptrBuff = buffer;
	}

	VkBufferCreateInfo buffInfo = {};
	buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffInfo.size = size;
	buffInfo.usage = usageFlags;
	buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult result = vkCreateBuffer(device, &buffInfo, nullptr, ptrBuff);
	if (result != VK_SUCCESS)
	{
		return result;
	};

	vkGetBufferMemoryRequirements(device, *ptrBuff, reqs);

	if (!buffer)
	{
		vkDestroyBuffer(device, *ptrBuff, nullptr);
	}

	return VK_SUCCESS;
}


static bool checkSupportForExt(
	const char** requiredExtensions,
	uint32_t requiredExtensionsNum,
	const VkExtensionProperties* supportedExtensions,
	uint32_t supportedExtensionsNum)
{
	uint32_t requiredExtensionsCount = 0;
	uint8_t* requiredExtFlags = new uint8_t[requiredExtensionsNum];
	memset(requiredExtFlags, 0, sizeof(uint8_t) * requiredExtensionsNum);

	for (uint32_t i = 0; i < supportedExtensionsNum; i++)
	{
		const char* extName = supportedExtensions[i].extensionName;
		for (uint32_t j = 0; j < requiredExtensionsNum; j++)
		{
			if (requiredExtFlags[j] != 1 && strcmp(extName, requiredExtensions[j]) == 0)
			{
				requiredExtFlags[j] = 1;
				requiredExtensionsCount++;
				break;
			}
		}

		if (requiredExtensionsCount == requiredExtensionsNum)
		{
			delete[] requiredExtFlags;
			return true;
		}
	}
	delete[] requiredExtFlags;
	return false;
}

VkDebugUtilsMessengerEXT setupDebug(
	VkInstance instance)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = vbDebugVal;
	createInfo.pUserData = nullptr; // Optional
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	VkDebugUtilsMessengerEXT debug;
	EXIT_ON_VK_ERROR(func(instance, &createInfo, nullptr, &debug));
	return debug;
}

static VkBool32 vbDebugVal(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		OutputDebugStringA("validation layer: ");
		OutputDebugStringA(pCallbackData->pMessage);
		OutputDebugStringA("\n");
	}
	return VK_FALSE;
}

static VkInstance createInstance()
{
	uint32_t propCount;
	EXIT_ON_VK_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &propCount, nullptr));
	vector<VkExtensionProperties> instExtSupported(propCount);
	EXIT_ON_VK_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &propCount, instExtSupported.data()));
	if (!checkSupportForExt(instExt, sizeof(instExt) / sizeof(const char*), instExtSupported.data(), instExtSupported.size()))
	{
		MessageBox(NULL, L"\nUnsupported extension found! \n", NULL, MB_OK);
		exit(-1);
	}

	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = vbDebugVal;

	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.pNext = NULL;


	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;;
	instanceInfo.enabledExtensionCount = sizeof(instExt) / sizeof(const char*);
	instanceInfo.ppEnabledExtensionNames = instExt;
	instanceInfo.enabledLayerCount = sizeof(vaLayers) / sizeof(const char*);
	instanceInfo.ppEnabledLayerNames = vaLayers;
	instanceInfo.pApplicationInfo = &appInfo;

	VkInstance instance = VK_NULL_HANDLE;
	EXIT_ON_VK_ERROR(vkCreateInstance(&instanceInfo, nullptr, &instance));
	return instance;
}

static VkSurfaceKHR createSurface(
	VkInstance instance, 
	HINSTANCE hinstance, 
	HWND hwnd)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkWin32SurfaceCreateInfoKHR surfInfo = {};
	surfInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfInfo.hinstance = hinstance;
	surfInfo.hwnd = hwnd;

	EXIT_ON_VK_ERROR(vkCreateWin32SurfaceKHR(instance, &surfInfo, nullptr, &surface));
	return surface;
}

static QueuesIdx createQueueIndecies(
	VkInstance instance, 
	VkPhysicalDevice dev,
	VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

	QueuesIdx queues = { -1, -1 };
	for (int64_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queues.grahicsIdx == -1 && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)))
		{
			queues.grahicsIdx = i;
		}

		VkBool32 surfaceSupport;
		EXIT_ON_VK_ERROR(vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &surfaceSupport));
		if (queues.presentationIdx == -1 && surfaceSupport)
		{
			queues.presentationIdx = i;
		}

		if (queues.presentationIdx != -1 && queues.grahicsIdx != -1)
		{
			break;
		}
	}

	if (queues.presentationIdx == -1 || queues.grahicsIdx == -1)
	{
		MessageBox(NULL, L"Required queue is not supported in the system \n", NULL, MB_OK);
		exit(-1);
	}
	return queues;
}

static VkDevice createLogicalDevice(
	VkPhysicalDevice physicalDev,
	QueuesIdx queuesIdx)
{
	uint32_t propCount;
	EXIT_ON_VK_ERROR(vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &propCount, nullptr));
	vector<VkExtensionProperties> devExtSupported(propCount);
	EXIT_ON_VK_ERROR(vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &propCount, devExtSupported.data()));

	if (!checkSupportForExt(devExt, sizeof(devExt) / sizeof(const char*), devExtSupported.data(), devExtSupported.size()))
	{
		MessageBox(NULL, L"\nUnsupported extension found! \n", NULL, MB_OK);
		exit(-1);
	}

	VkDevice device;
	float prioGfx[] = { 1.0f, 1.0f }, prioPres[] = { 1.0f };
	uint32_t familyCount = 1;
	VkDeviceQueueCreateInfo queueInfos[2] = {};
	queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfos[0].queueFamilyIndex = queuesIdx.grahicsIdx;
	queueInfos[0].queueCount = 2;
	queueInfos[0].pQueuePriorities = prioGfx;
	if (queuesIdx.grahicsIdx != queuesIdx.presentationIdx)
	{
		familyCount = 2;
		queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfos[1].queueFamilyIndex = queuesIdx.presentationIdx;
		queueInfos[1].queueCount = 1;
		queueInfos[1].pQueuePriorities = prioPres;
	}

	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = VK_TRUE;
	features.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.pQueueCreateInfos = queueInfos;
	info.queueCreateInfoCount = familyCount;
	info.enabledLayerCount = 1;
	info.ppEnabledLayerNames = vaLayers;
	info.enabledExtensionCount = 1;
	info.ppEnabledExtensionNames = devExt;
	info.pEnabledFeatures = &features;
	EXIT_ON_VK_ERROR(vkCreateDevice(physicalDev, &info, nullptr, &device));
	return device;
}

static VkPhysicalDevice pickPhysicalDevice(
	VkInstance instance)
{
	uint32_t count;
	EXIT_ON_VK_ERROR(vkEnumeratePhysicalDevices(instance, &count, nullptr));
	vector<VkPhysicalDevice> physicalDevices(count, VK_NULL_HANDLE);
	EXIT_ON_VK_ERROR(vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data()));

	for (VkPhysicalDevice dev : physicalDevices)
	{
		VkPhysicalDeviceProperties props = {};
		VkPhysicalDeviceFeatures features = {};
		vkGetPhysicalDeviceProperties(dev, &props);
		vkGetPhysicalDeviceFeatures(dev, &features);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.shaderFloat64 && features.samplerAnisotropy && features.fillModeNonSolid)
		{
			return dev;
		}
	}
	MessageBox(NULL, L"\nDiscrete GPU that support requested features does not exist! \n", NULL, MB_OK);
	exit(-1);
	return VK_NULL_HANDLE;
}

static SwapchainInfo querySwapChainSupport(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface)
{
	SwapchainInfo info;
	EXIT_ON_VK_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &info.capabilities));

	uint32_t formatCount;
	EXIT_ON_VK_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
	if (formatCount != 0)
	{
		info.formats.resize(formatCount);
		EXIT_ON_VK_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, info.formats.data()));
	}

	uint32_t presentModeCount;
	EXIT_ON_VK_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
	if (presentModeCount != 0)
	{
		info.presentModes.resize(presentModeCount);
		EXIT_ON_VK_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, info.presentModes.data()));
	}

	return info;
}

static VkSwapchainKHR createSwapchain(
	VkDevice device, 
	VkSurfaceKHR surface,
	const SwapchainInfo& swapchainInfo,
	const QueuesIdx& queuesIdx, 
	VkFormat* chosenFormat)
{
	size_t i;
	for (i = 0; i < swapchainInfo.formats.size(); i++)
	{
		if (swapchainInfo.formats[i].format == SURFACE_FORMAT)
		{
			break;
		}
	}
	if (i == swapchainInfo.formats.size())
	{
		MessageBox(NULL, L"Unsupported VK_FORMAT_R8G8B8A8_UNORM! \n", NULL, MB_OK);
		exit(-1);
	}

	if (chosenFormat)
	{
		*chosenFormat = swapchainInfo.formats[i].format;
	}
	uint32_t imgCount = swapchainInfo.capabilities.minImageCount + 1 <= swapchainInfo.capabilities.maxImageCount ?
		swapchainInfo.capabilities.minImageCount + 1 : swapchainInfo.capabilities.minImageCount;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.imageArrayLayers = 1;
	info.imageExtent = swapchainInfo.capabilities.currentExtent;
	info.minImageCount = imgCount;
	info.preTransform = swapchainInfo.capabilities.currentTransform;
	info.imageFormat = swapchainInfo.formats[i].format;
	info.imageColorSpace = swapchainInfo.formats[i].colorSpace;
	info.clipped = VK_FALSE;
	info.oldSwapchain = VK_NULL_HANDLE;

	if (queuesIdx.grahicsIdx != queuesIdx.presentationIdx)
	{
		uint32_t queueFamilies[] = { queuesIdx.grahicsIdx, queuesIdx.presentationIdx };
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queueFamilies;
	}
	else
	{
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.queueFamilyIndexCount = 0; // Optional
		info.pQueueFamilyIndices = nullptr; // Optional
	}

	EXIT_ON_VK_ERROR(vkCreateSwapchainKHR(device, &info, nullptr, &swapchain));

	return swapchain;
}

Texture createTexture2D(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageUsageFlags usage,
	bool isCubemap)
{

	Texture texture = {};
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.flags = isCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = isCubemap ? 6 : 1 ;
	imageInfo.format = format;
	imageInfo.tiling = (usage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0 || isCubemap ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	EXIT_ON_VK_ERROR(vkCreateImage(device, &imageInfo, nullptr, &texture.texImage));

	VkMemoryRequirements memRequirements = {};
	vkGetImageMemoryRequirements(device, texture.texImage, &memRequirements);
	texture.alignment = memRequirements.alignment;
	texture.memory = allocateBuffer_temp(device, physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	EXIT_ON_VK_ERROR(vkBindImageMemory(device, texture.texImage, texture.memory, 0));

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture.texImage;
	viewInfo.viewType = isCubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = isCubemap ? 6 : 1;
	EXIT_ON_VK_ERROR(vkCreateImageView(device, &viewInfo, nullptr, &texture.texView));
	return texture;
}

static VkDeviceMemory allocateBuffer_temp(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkMemoryRequirements memRequirements,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	uint32_t i;
	for (i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((memRequirements.memoryTypeBits & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			break;
		}
	}

	if (i == memProperties.memoryTypeCount)
	{
		return nullptr;
	}

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = i;

	VkDeviceMemory devMem;
	EXIT_ON_VK_ERROR(vkAllocateMemory(device, &allocInfo, nullptr, &devMem));
	return devMem;
}

static std::vector<VkImage> createSwapchainImages(
	VkDevice device,
	VkSwapchainKHR swapchain)
{
	std::vector<VkImage> images;
	uint32_t imageCount;
	EXIT_ON_VK_ERROR(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
	images.resize(imageCount);
	EXIT_ON_VK_ERROR(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

	return images;
}

static std::vector<VkImageView> createSwapchainImageViews(
	VkDevice					device, 
	const std::vector<VkImage>& images,
	const SwapchainInfo&		swcInfo,
	const VkFormat&				imgFormat)
{

	std::vector<VkImageView> views(images.size());
	for (size_t i = 0; i < images.size(); i++)
	{
		VkImageViewCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgInfo.image = images[i];
		imgInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgInfo.format = imgFormat;
		imgInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgInfo.subresourceRange.baseMipLevel = 0;
		imgInfo.subresourceRange.levelCount = 1;
		imgInfo.subresourceRange.baseArrayLayer = 0;
		imgInfo.subresourceRange.layerCount = 1;

		EXIT_ON_VK_ERROR(vkCreateImageView(device, &imgInfo, nullptr, &views[i]));
	}

	return views;
}


static VkCommandPool createCommandPool(
	VkDevice device, 
	uint32_t queueIdx)
{
	VkCommandPool cmdPool;
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = queueIdx;

	EXIT_ON_VK_ERROR(vkCreateCommandPool(device, &info, nullptr, &cmdPool));
	return cmdPool;
}

static std::vector<VkCommandBuffer> createCommandBuffers(
	VkDevice device,
	VkCommandPool cmdPool,
	uint32_t bufferCount)
{
	std::vector<VkCommandBuffer> cmdBuffers(bufferCount);

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandBufferCount = bufferCount;
	info.commandPool = cmdPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	EXIT_ON_VK_ERROR(vkAllocateCommandBuffers(device, &info, cmdBuffers.data()));
	return cmdBuffers;
}

static VkRenderPass createRenderPass(
	VkDevice device,
	VkFormat imgFormat)
{
	VkRenderPass renderPass;
	VkAttachmentDescription attachmentDesc[3] = {};

	attachmentDesc[2].format = VK_FORMAT_D32_SFLOAT;
	attachmentDesc[2].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	
	attachmentDesc[1].format = imgFormat;
	attachmentDesc[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc[1].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	attachmentDesc[0].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	attachmentDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[0].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDesc[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 1;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef[2] = {};
	depthAttachmentRef[1].attachment = 2;
	depthAttachmentRef[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	depthAttachmentRef[0].attachment = 0;
	depthAttachmentRef[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass[2] = {};
	subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass[1].colorAttachmentCount = 1;
	subpass[1].pColorAttachments = &colorAttachmentRef;
	subpass[1].pDepthStencilAttachment = &depthAttachmentRef[0];

	subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass[0].pDepthStencilAttachment = &depthAttachmentRef[1];

	VkSubpassDependency dependency[3] = {};

	dependency[2].srcSubpass = 0;
	dependency[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency[2].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency[2].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	dependency[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[1].dstSubpass = 1;
	dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency[1].srcAccessMask = 0;
	dependency[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[0].dstSubpass = 0;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency[0].srcAccessMask = 0;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 3;
	info.pAttachments = attachmentDesc;
	info.subpassCount = 2;
	info.pSubpasses = subpass;
	info.dependencyCount = 3;
	info.pDependencies = dependency;

	EXIT_ON_VK_ERROR(vkCreateRenderPass(device, &info, nullptr, &renderPass));

	return renderPass;
}

static DepthBufferBundle createDepthBuffer(
	VkDevice device, 
	VkPhysicalDevice physicalDevice, 
	uint32_t width,
	uint32_t height,
	bool isShadowmap)
{
	DepthBufferBundle bundleOut = {};

	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);
	if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)
	{
		exitOnError(L"Unsupported VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT for optimal features\n");
	}

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = isShadowmap ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_D32_SFLOAT_S8_UINT ;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = isShadowmap ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	EXIT_ON_VK_ERROR(vkCreateImage(device, &imageInfo, nullptr, &bundleOut.depthImage));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, bundleOut.depthImage, &memRequirements);
	bundleOut.depthImageMemory = allocateBuffer_temp(device, physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	EXIT_ON_VK_ERROR(vkBindImageMemory(device, bundleOut.depthImage, bundleOut.depthImageMemory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = bundleOut.depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = isShadowmap ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_D32_SFLOAT_S8_UINT;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	EXIT_ON_VK_ERROR(vkCreateImageView(device, &viewInfo, nullptr, &bundleOut.depthImageView));
	return bundleOut;
}

static std::vector<VkFramebuffer> createFramebuffers(
	VkDevice device,
	VkRenderPass renderPass, 
	VkImageView depthView,
	VkImageView shadowmapView,
	const std::vector<VkImageView> imgViews,
	const SwapchainInfo& swcInfo)
{
	vector<VkFramebuffer> framebuffers(imgViews.size());
	for (size_t i = 0; i < imgViews.size(); i++)
	{
		VkImageView views[3] = {depthView, imgViews[i], shadowmapView };

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = renderPass;
		info.attachmentCount = 3;
		info.pAttachments = views;
		info.width = swcInfo.capabilities.currentExtent.width;
		info.height = swcInfo.capabilities.currentExtent.height;
		info.layers = 1;
		EXIT_ON_VK_ERROR(vkCreateFramebuffer(device, &info, nullptr, &framebuffers[i]));
	}
	return framebuffers;
}