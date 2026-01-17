#include "Renderer.hpp"
#include "VulkanResourcesInternal.hpp"
#include "errors.hpp"
#include "CommonShapes.hpp"
using namespace std;
static constexpr uint32_t MAX_UBO_POOL_SIZE = 65536; // value from vulkan spec v1.4
static constexpr uint32_t UBO_BUFFER_RESOURCE_TYPE = 0;

static constexpr uint32_t MAIN_COMPUTE = 0;
#define GET_BASE_SHAPE(name) baseShapesTable[static_cast<uint32_t>(name)]
enum class ComputeLayout
{
    RenderTexture = 0,
    ComputeTexture,
    StorageImageCount
};

static constexpr uint32_t CL_RENDER_TEXTURE = static_cast<uint32_t>(ComputeLayout::RenderTexture);
static constexpr uint32_t CL_COMPUTE_TEXTURE = static_cast<uint32_t>(ComputeLayout::ComputeTexture);
static constexpr uint32_t CL_STORAGE_IMAGES = static_cast<uint32_t>(ComputeLayout::StorageImageCount);

enum class GraphicsLayout
{
    GlobalUbo = 0,
    ObjectUbo,
    SampledImages,
    GfxEntryCount,
};

static constexpr uint32_t GFX_CAMERA_UBO = static_cast<uint32_t>(GraphicsLayout::GlobalUbo);
static constexpr uint32_t GFX_OBJECT_UBO = static_cast<uint32_t>(GraphicsLayout::ObjectUbo);
static constexpr uint32_t GFX_SAMPLED_IMAGE = static_cast<uint32_t>(GraphicsLayout::SampledImages);
static constexpr uint32_t GFX_ENTRY_COUNT = static_cast<uint32_t>(GraphicsLayout::GfxEntryCount);

const Geometry* baseShapesTable[]
{
    &commonBox
};

Renderer::Renderer(
    HINSTANCE hinstance,
    HWND hwnd,
    VkDeviceSize stagingSize)
	:
	windowHwnd(hwnd), compiler(), pipelines(1), maxUboPoolSize(65536)
{
	createVulkanResources(&vkResources, hinstance, windowHwnd);

    int64_t errCode = allocateBuffer(vkResources.device, vkResources.physicalDevice, stagingSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);
    EXIT_ON_VK_ERROR(vkMapMemory(vkResources.device, stagingBuffer.deviceMemory, 0, stagingBuffer.requirements.size, 0, (void**) & stagingPtr));
    CreateControllingStructs();

    VulkanPipelineData* pipeline = &pipelines.back();
    CreateComputeLayout(&pipeline->descriptorSetLayout, &pipeline->pipelineLayout);
    CreateComputePipeline(pipeline->pipelineLayout, &pipeline->pipeline);
    InitComputeSets(pipeline);
}

void Renderer::BeginRendering()
{
    EXIT_ON_VK_ERROR(vkWaitForFences(vkResources.device, 1, &vkResources.gfxQueueFinished, VK_TRUE, UINT64_MAX));
    EXIT_ON_VK_ERROR(vkResetFences(vkResources.device, 1, &vkResources.gfxQueueFinished));
    EXIT_ON_VK_ERROR(vkAcquireNextImageKHR(vkResources.device, vkResources.swapchain, UINT64_MAX, vkResources.imgReady, VK_NULL_HANDLE, &imageIndex));

    VkCommandBufferBeginInfo cmdBuffInfo = {};
    cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    EXIT_ON_VK_ERROR(vkResetCommandBuffer(vkResources.cmdBuffer, 0));
    EXIT_ON_VK_ERROR(vkBeginCommandBuffer(vkResources.cmdBuffer, &cmdBuffInfo));

    vkCmdBeginRenderPass(vkResources.cmdBuffer, &renderPassInfos[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vkResources.swapchainInfo.capabilities.currentExtent.width;
    viewport.height = vkResources.swapchainInfo.capabilities.currentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkResources.cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vkResources.swapchainInfo.capabilities.currentExtent;
    vkCmdSetScissor(vkResources.cmdBuffer, 0, 1, &scissor);
}

int64_t Renderer::CreateGraphicsPipeline(
    uint8_t lightCount,
    const std::vector<TextureDim>& textureDims,
    uint64_t* pipelineId)
{

    if (textureDims.size() > 0xFFu)
    {
        return -2;
    }

    pipelines.push_back({});
    VulkanPipelineData* pipeline = &pipelines.back();
    pipeline->lightCount = lightCount;
    pipeline->texDims = textureDims;

    CreateBasicGraphicsLayout(pipeline->texDims.size(), &pipeline->descriptorSetLayout, &pipeline->pipelineLayout);
    CreateBasicGraphicsVkPipeline(pipeline->pipelineLayout, &pipeline->pipeline, lightCount, pipeline->texDims.size());
    CreateGraphicsSets(pipeline);
    CreateSampler(pipeline);
    for (size_t i = 0; i < textureDims.size(); i++)
    {
        Texture tex = createTexture2D(vkResources.device, vkResources.physicalDevice,
            pipeline->texDims[i].width, pipeline->texDims[i].height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT);
        pipeline->textures.push_back(std::move(tex));
    }


    SetImageSets(pipeline);
    *pipelineId = pipelines.size() - 1;
    return 0;
}

int64_t Renderer::UpdateUboMemory(
    uint64_t poolId,
    uint64_t uboId,
    const char* buff)
{
    if (poolId > uboPoolEntries.size())
    {
        return -1;
    }

    UboPoolEntry* uboPoolEntry = &uboPoolEntries[poolId - 1];
    if (uboId > uboPoolEntry->uboEntries.size())
    {
        return -2;
    }
    UboEntry* entry = &uboPoolEntry->uboEntries[uboId - 1];
    char* uboMem = uboPoolEntry->memoryMap + entry->bufferIdx * MAX_UBO_POOL_SIZE + entry->bufferOffset;
    memcpy(uboMem, buff, uboPoolEntry->uboPool.bufferInfos[entry->resourceId].size);
    return 0;
}

int64_t Renderer::CreateUboPool(
    VkDeviceSize globalUboSize,
    VkDeviceSize localUboSize,
    VkDeviceSize poolSize, 
    uint64_t* createdPoolId)
{
    uboPoolEntries.push_back({});
    UboPoolEntry* newPoolEntry = &uboPoolEntries.back();
    if(globalUboSize == 0 && localUboSize == 0)
    {
        return VULKAN_RENDERER_ERROR;
    }

    if (poolSize == 0)
    {
        poolSize = MAX_UBO_POOL_SIZE;
    }

    poolSize += MAX_UBO_POOL_SIZE - poolSize % MAX_UBO_POOL_SIZE ;
    newPoolEntry->uboPool = {};
    EXIT_ON_VK_ERROR(allocateMemoryPool(vkResources.device, vkResources.physicalDevice, poolSize, {MAX_UBO_POOL_SIZE, globalUboSize, localUboSize},
        {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &newPoolEntry->uboPool));

    while (newPoolEntry->uboPool.currOffset < newPoolEntry->uboPool.poolSize)
    {
        EXIT_ON_VK_ERROR(createBufferInPool(0, vkResources.device, &newPoolEntry->uboPool));
    }
    EXIT_ON_VK_ERROR(vkMapMemory(vkResources.device, newPoolEntry->uboPool.deviceMemory, 0, poolSize, 0, (void**) &newPoolEntry->memoryMap));
    
    *createdPoolId = uboPoolEntries.size();
    return 0;
}

int64_t Renderer::AllocateUboResource(
    uint64_t poolId, 
    uint64_t resourceId,
    uint64_t* allocatedUboId)
{
    if (poolId > uboPoolEntries.size())
    {
        return -1;
    }

    UboPoolEntry* uboPoolEntry = &uboPoolEntries[poolId - 1];
    if (resourceId == 0 || resourceId > uboPoolEntry->uboPool.resourceReqs.size())
    {
        return -1;
    }

    const VkMemoryRequirements& resourceReqs = uboPoolEntry->uboPool.resourceReqs[resourceId];
    const VkBufferCreateInfo resourceInfo = uboPoolEntry->uboPool.bufferInfos[resourceId];
    VkDeviceSize memoryUpdateSize;
    UboEntry uboDesc = {};
    uboDesc.resourceId = resourceId;
    uboDesc.bufferIdx = uboPoolEntry->bufferIdx;
    SET_UBO_ENTRY_ALIVE(&uboDesc);

    VkResult result = findOffsetInBuffer(uboPoolEntry->bufferOffset, resourceReqs.alignment, resourceReqs.size,
                        uboPoolEntry->uboPool.poolSize, resourceInfo.size, &uboDesc.bufferOffset, &memoryUpdateSize);
    if (result == VK_ERROR_TOO_MANY_OBJECTS)
    {
        if (uboPoolEntry->bufferIdx + 1 >= uboPoolEntry->uboPool.boundBuffers.size())
        {
            return VK_ERROR_TOO_MANY_OBJECTS;
        }
        uboPoolEntry->bufferIdx++;
        uboDesc.bufferIdx = uboPoolEntry->bufferIdx;
        uboPoolEntry->bufferOffset = 0;
        EXIT_ON_VK_ERROR(findOffsetInBuffer(uboPoolEntry->bufferOffset, resourceReqs.alignment, resourceReqs.size,
            uboPoolEntry->uboPool.poolSize, resourceInfo.size, &uboDesc.bufferOffset, &memoryUpdateSize));
    }
    uboPoolEntry->uboEntries.push_back(uboDesc);
    uboPoolEntry->bufferOffset += memoryUpdateSize;

    *allocatedUboId = uboPoolEntry->uboEntries.size();

    return 0;
}

int64_t Renderer::BindUboPoolToPipeline(
    uint64_t pipelineId,
    uint64_t uboPoolId,
    uint64_t globalUboId)
{
    if (pipelineId > pipelines.size())
    {
        return -1;
    }
    if (uboPoolId > uboPoolEntries.size())
    {
        return -2;
    }

    if (pipelines[pipelineId].maxSets < uboPoolEntries[uboPoolId - 1].uboPool.boundBuffers.size())
    {
        return -3;
    }

    VulkanPipelineData* pipelineData = &pipelines[pipelineId];
    UboPoolEntry* uboPoolEntry = &uboPoolEntries[uboPoolId - 1];
    UboEntry* camUbo = &uboPoolEntry->uboEntries[globalUboId - 1];

    pipelineData->boundPoolId = uboPoolId;
    pipelineData->boundGlobalUboId = globalUboId;

    for (size_t i = 0; i < uboPoolEntries[uboPoolId - 1].uboPool.boundBuffers.size(); i++)
    {
        VkWriteDescriptorSet updateGfxSet[2] = {};
        VkDescriptorBufferInfo buffInfos[2] = {};

        buffInfos[GFX_CAMERA_UBO].buffer = uboPoolEntry->uboPool.boundBuffers[camUbo->bufferIdx];
        buffInfos[GFX_CAMERA_UBO].offset = 0;
        buffInfos[GFX_CAMERA_UBO].range = uboPoolEntry->uboPool.bufferInfos[GFX_CAMERA_UBO].size;

        buffInfos[GFX_OBJECT_UBO].buffer = uboPoolEntry->uboPool.boundBuffers[i];
        buffInfos[GFX_OBJECT_UBO].offset = 0;
        buffInfos[GFX_OBJECT_UBO].range = uboPoolEntry->uboPool.bufferInfos[GFX_OBJECT_UBO].size;

        updateGfxSet[GFX_CAMERA_UBO].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        updateGfxSet[GFX_CAMERA_UBO].dstSet = pipelineData->sets[i];
        updateGfxSet[GFX_CAMERA_UBO].dstBinding = GFX_CAMERA_UBO;
        updateGfxSet[GFX_CAMERA_UBO].dstArrayElement = 0;
        updateGfxSet[GFX_CAMERA_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        updateGfxSet[GFX_CAMERA_UBO].descriptorCount = 1;
        updateGfxSet[GFX_CAMERA_UBO].pBufferInfo = &buffInfos[GFX_CAMERA_UBO];

        updateGfxSet[GFX_OBJECT_UBO].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        updateGfxSet[GFX_OBJECT_UBO].dstSet = pipelineData->sets[i];
        updateGfxSet[GFX_OBJECT_UBO].dstBinding = GFX_OBJECT_UBO;
        updateGfxSet[GFX_OBJECT_UBO].dstArrayElement = 0;
        updateGfxSet[GFX_OBJECT_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        updateGfxSet[GFX_OBJECT_UBO].descriptorCount = 1;
        updateGfxSet[GFX_OBJECT_UBO].pBufferInfo = &buffInfos[GFX_OBJECT_UBO];

        vkUpdateDescriptorSets(vkResources.device, 2, updateGfxSet, 0, nullptr);
    }
    return 0;
}

void Renderer::Render(
    uint64_t meshCollectionId,
    uint64_t pipelineId,
    const std::vector<RenderItem>& renderItems)
{
    MeshCollection* collection = &meshCollections[meshCollectionId - 1];
    VkDeviceSize offsets[] = { 0 };
    UboPoolEntry* uboPool = &uboPoolEntries[pipelines[pipelineId].boundPoolId - 1];

    vkCmdBindPipeline(vkResources.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[pipelineId].pipeline);
    vkCmdBindVertexBuffers(vkResources.cmdBuffer, 0, 1, &collection->vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(vkResources.cmdBuffer, collection->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    for (size_t i = 0; i < renderItems.size(); i++)
    {
        uint32_t setDynamicRange[2] = { uboPool->uboEntries[pipelines[pipelineId].boundGlobalUboId - 1].bufferOffset, 
                                        uboPool->uboEntries[renderItems[i].transformUboId - 1].bufferOffset};
        vkCmdBindDescriptorSets(vkResources.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[pipelineId].pipelineLayout,
            uboPool->uboEntries[renderItems[i].transformUboId - 1].bufferIdx, 1, pipelines[pipelineId].sets.data(), 2, setDynamicRange);

        vkCmdDrawIndexed(vkResources.cmdBuffer, collection->indexCount[renderItems[i].indexCountOffset],
            1, collection->ibOffset[renderItems[i].indexOffset], collection->vbOffset[renderItems[i].vertexOffset], 1);
    }

}



void Renderer::Present()
{
    vkCmdEndRenderPass(vkResources.cmdBuffer);

    vkCmdPipelineBarrier(vkResources.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 0, nullptr);

    vkCmdBindPipeline(vkResources.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[MAIN_COMPUTE].pipeline);
    vkCmdBindDescriptorSets(vkResources.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[MAIN_COMPUTE].pipelineLayout,
        0, 1, &pipelines[MAIN_COMPUTE].sets[imageIndex], 0, nullptr);

    uint32_t x_dispatch = vkResources.swapchainInfo.capabilities.currentExtent.width;
    uint32_t y_dispatch = vkResources.swapchainInfo.capabilities.currentExtent.height;

    vkCmdDispatch(vkResources.cmdBuffer, x_dispatch, y_dispatch, 1);

    vkCmdPipelineBarrier(vkResources.cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 2, transferBarriers.data() + imageIndex * 2);

    vkCmdCopyImage(vkResources.cmdBuffer, vkResources.computeTextures[imageIndex].texImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vkResources.swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &computeSwapchainCopy);


    vkCmdPipelineBarrier(vkResources.cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr, 2, restoreBarriers.data() + imageIndex * 2);

    EXIT_ON_VK_ERROR(vkEndCommandBuffer(vkResources.cmdBuffer));
    vkQueueSubmit(vkResources.graphicsQueue, 1, &submitInfo, vkResources.gfxQueueFinished);


    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &vkResources.swapchain;
    info.pImageIndices = &imageIndex;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vkResources.renderingFinished;
    EXIT_ON_VK_ERROR(vkQueuePresentKHR(vkResources.presentationQueue, &info));

}

int64_t Renderer::CreateMeshCollection(
    const std::vector<GeometryEntry>& geometryEntries,
    uint64_t* createdMeshId)
{
    if (meshCollections.size() == 0xFFFE)
    {
        return VULKAN_RENDERER_ERROR;
    }

    MeshCollection newColletion = {};
    vector<CpuBuffer> vertexBuffers(geometryEntries.size());
    vector<CpuBuffer> indexBuffers(geometryEntries.size());
    newColletion.vbOffset.resize(geometryEntries.size());
    newColletion.ibOffset.resize(geometryEntries.size());
    newColletion.indexCount.resize(geometryEntries.size());

    VkDeviceSize vbSize = 0;
    VkDeviceSize ibSize = 0;
    for (size_t i = 0; i < geometryEntries.size(); i++)
    {
        newColletion.vbOffset[i] = vbSize;
        newColletion.ibOffset[i] = ibSize;

        if (geometryEntries[i].gType == GeometryType::Mesh)
        {
            exitOnError(L"GeometryType::Mesh is currently not supported\n");
        }
        else
        {
            vbSize += GET_BASE_SHAPE(geometryEntries[i].gType)->vertecies->size() * sizeof(CommonVertex);
            ibSize += GET_BASE_SHAPE(geometryEntries[i].gType)->indicies->size() * sizeof(uint16_t);

            vertexBuffers[i] = {(const char*)GET_BASE_SHAPE(geometryEntries[i].gType)->vertecies->data(),
                                GET_BASE_SHAPE(geometryEntries[i].gType)->vertecies->size() * sizeof(CommonVertex) };
            indexBuffers[i] = { (const char*)GET_BASE_SHAPE(geometryEntries[i].gType)->indicies->data(),
                               GET_BASE_SHAPE(geometryEntries[i].gType)->indicies->size() * sizeof(uint16_t) };
            newColletion.indexCount[i] = GET_BASE_SHAPE(geometryEntries[i].gType)->indicies->size();
        }
    }

    if(allocateBuffer(vkResources.device, vkResources.physicalDevice, vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &newColletion.vertexBuffer) != VK_RESOURCES_OK)
    {
        goto cleanup;
    }

    if(allocateBuffer(vkResources.device, vkResources.physicalDevice, ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &newColletion.indexBuffer) != VK_RESOURCES_OK)
    {
        goto cleanup;
    }

    EXIT_ON_VK_ERROR(uploadStagingData(vkResources.sideCmdBuffer, vkResources.sideQueue, stagingBuffer.buffer, stagingPtr,
        stagingBuffer.requirements.size, newColletion.vertexBuffer.buffer, vertexBuffers));

    EXIT_ON_VK_ERROR(uploadStagingData(vkResources.sideCmdBuffer, vkResources.sideQueue, stagingBuffer.buffer, stagingPtr,
        stagingBuffer.requirements.size, newColletion.indexBuffer.buffer, indexBuffers));

    meshCollections.push_back(newColletion);
    *createdMeshId = meshCollections.size();
    return 0;

cleanup:
    if (newColletion.vertexBuffer.buffer)
    {
        vkDestroyBuffer(vkResources.device, newColletion.vertexBuffer.buffer, nullptr);
    }
    if (newColletion.indexBuffer.buffer)
    {
        vkDestroyBuffer(vkResources.device, newColletion.indexBuffer.buffer, nullptr);
    }
    if (newColletion.vertexBuffer.deviceMemory)
    {
        vkFreeMemory(vkResources.device, newColletion.vertexBuffer.deviceMemory, nullptr);
    }
    if (newColletion.indexBuffer.deviceMemory)
    {
        vkFreeMemory(vkResources.device, newColletion.indexBuffer.deviceMemory, nullptr);
    }
    return VULKAN_RENDERER_ERROR;
}

void Renderer::CreateControllingStructs()
{
    static 	VkClearValue clearColor[2];
    clearColor[1].color = { {0.3f, 0.3f, 1.0f, 1.0f} };
    clearColor[0].depthStencil = { 1.0f, 0 };

    renderPassInfos.resize(vkResources.swapchainFramebuffers.size());
    for (size_t i = 0; i < renderPassInfos.size(); i++)
    {
        renderPassInfos[i] = {};
        renderPassInfos[i].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfos[i].renderPass = vkResources.renderPass;
        renderPassInfos[i].framebuffer = vkResources.swapchainFramebuffers[i];
        renderPassInfos[i].renderArea.offset = { 0, 0 };
        renderPassInfos[i].renderArea.extent = vkResources.swapchainInfo.capabilities.currentExtent;
        renderPassInfos[i].clearValueCount = 2;
        renderPassInfos[i].pClearValues = clearColor;
    }

    transferBarriers.resize(vkResources.swapchainImages.size() * 2);
    restoreBarriers.resize(vkResources.swapchainImages.size() * 2);
    for (size_t i = 0; i < vkResources.swapchainImages.size(); i++)
    {
        transferBarriers[2 * i] = {};
        transferBarriers[2 * i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transferBarriers[2 * i].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        transferBarriers[2 * i].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        transferBarriers[2 * i].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        transferBarriers[2 * i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transferBarriers[2 * i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarriers[2 * i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarriers[2 * i].image = vkResources.swapchainImages[i];
        transferBarriers[2 * i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        transferBarriers[2 * i].subresourceRange.baseMipLevel = 0;
        transferBarriers[2 * i].subresourceRange.levelCount = 1;
        transferBarriers[2 * i].subresourceRange.baseArrayLayer = 0;
        transferBarriers[2 * i].subresourceRange.layerCount = 1;

        transferBarriers[2 * i + 1] = {};
        transferBarriers[2 * i + 1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transferBarriers[2 * i + 1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        transferBarriers[2 * i + 1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        transferBarriers[2 * i + 1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        transferBarriers[2 * i + 1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        transferBarriers[2 * i + 1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarriers[2 * i + 1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarriers[2 * i + 1].image = vkResources.computeTextures[i].texImage;
        transferBarriers[2 * i + 1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        transferBarriers[2 * i + 1].subresourceRange.baseMipLevel = 0;
        transferBarriers[2 * i + 1].subresourceRange.levelCount = 1;
        transferBarriers[2 * i + 1].subresourceRange.baseArrayLayer = 0;
        transferBarriers[2 * i + 1].subresourceRange.layerCount = 1;

        restoreBarriers[2 * i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        restoreBarriers[2 * i].srcAccessMask = 0;
        restoreBarriers[2 * i].dstAccessMask = 0;
        restoreBarriers[2 * i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        restoreBarriers[2 * i].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        restoreBarriers[2 * i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        restoreBarriers[2 * i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        restoreBarriers[2 * i].image = vkResources.swapchainImages[i];
        restoreBarriers[2 * i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        restoreBarriers[2 * i].subresourceRange.baseMipLevel = 0;
        restoreBarriers[2 * i].subresourceRange.levelCount = 1;
        restoreBarriers[2 * i].subresourceRange.baseArrayLayer = 0;
        restoreBarriers[2 * i].subresourceRange.layerCount = 1;

        restoreBarriers[2 * i + 1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        restoreBarriers[2 * i + 1].srcAccessMask = 0;
        restoreBarriers[2 * i + 1].dstAccessMask = 0;
        restoreBarriers[2 * i + 1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        restoreBarriers[2 * i + 1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        restoreBarriers[2 * i + 1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        restoreBarriers[2 * i + 1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        restoreBarriers[2 * i + 1].image = vkResources.computeTextures[i].texImage;
        restoreBarriers[2 * i + 1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        restoreBarriers[2 * i + 1].subresourceRange.baseMipLevel = 0;
        restoreBarriers[2 * i + 1].subresourceRange.levelCount = 1;
        restoreBarriers[2 * i + 1].subresourceRange.baseArrayLayer = 0;
        restoreBarriers[2 * i + 1].subresourceRange.layerCount = 1;
    }

    static VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkResources.cmdBuffer;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vkResources.imgReady;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vkResources.renderingFinished;

    computeSwapchainCopy = {};
    computeSwapchainCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    computeSwapchainCopy.srcSubresource.baseArrayLayer = 0;
    computeSwapchainCopy.srcSubresource.layerCount = 1;
    computeSwapchainCopy.srcSubresource.mipLevel = 0;
    computeSwapchainCopy.srcOffset = { 0, 0, 0 };
    computeSwapchainCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    computeSwapchainCopy.dstSubresource.baseArrayLayer = 0;
    computeSwapchainCopy.dstSubresource.layerCount = 1;
    computeSwapchainCopy.dstSubresource.mipLevel = 0;
    computeSwapchainCopy.dstOffset = { 0, 0, 0 };
    computeSwapchainCopy.extent = { vkResources.swapchainInfo.capabilities.currentExtent.width, vkResources.swapchainInfo.capabilities.currentExtent.height, 1 };
}

void Renderer::CreateComputePipeline(
    VkPipelineLayout pipelineLayout,
    VkPipeline* pipeline)
{
    ShaderOptions options = {};
    VkShaderModule computeShader = compiler.CompileShaderFromPath(vkResources.device, nullptr, "shaders/post_process.comp", "main", shaderc_compute_shader, options);

    VkPipelineShaderStageCreateInfo shaderInfo = {};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderInfo.module = computeShader;
    shaderInfo.pName = "main";
    shaderInfo.pSpecializationInfo = NULL;


    VkComputePipelineCreateInfo computeInfo = {};
    computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeInfo.layout = pipelineLayout;
    computeInfo.stage = shaderInfo;
    computeInfo.basePipelineHandle = VK_NULL_HANDLE;
    computeInfo.basePipelineIndex = -1;

    EXIT_ON_VK_ERROR(vkCreateComputePipelines(vkResources.device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, pipeline));

    vkDestroyShaderModule(vkResources.device, computeShader, nullptr);
}

void Renderer::CreateComputeLayout(
    VkDescriptorSetLayout* setLayout,
    VkPipelineLayout* pipelineLayout)
{
    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0].binding = CL_RENDER_TEXTURE;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = CL_COMPUTE_TEXTURE;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo setInfo = {};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.bindingCount = 2;
    setInfo.pBindings = bindings;
    EXIT_ON_VK_ERROR(vkCreateDescriptorSetLayout(vkResources.device, &setInfo, nullptr, setLayout));

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = setLayout;

    EXIT_ON_VK_ERROR(vkCreatePipelineLayout(vkResources.device, &layoutInfo, nullptr, pipelineLayout));
}



void Renderer::InitComputeSets(
    VulkanPipelineData* pipelineData)
{
    VkDescriptorPoolSize computePoolSize[1] = {};
    computePoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    computePoolSize[0].descriptorCount = CL_STORAGE_IMAGES * vkResources.renderTextures.size();
    pipelineData->maxSets = vkResources.renderTextures.size();

    VkDescriptorPoolCreateInfo computePoolDesc = {};
    computePoolDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    computePoolDesc.maxSets = pipelineData->maxSets;
    computePoolDesc.poolSizeCount = 1;
    computePoolDesc.pPoolSizes = computePoolSize;
    EXIT_ON_VK_ERROR(vkCreateDescriptorPool(vkResources.device, &computePoolDesc, nullptr, &pipelineData->descriptorPool));


    pipelineData->sets.resize(vkResources.renderTextures.size());
    vector<VkDescriptorSetLayout> layouts(vkResources.renderTextures.size(), pipelineData->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pipelineData->descriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    EXIT_ON_VK_ERROR(vkAllocateDescriptorSets(vkResources.device, &allocInfo, pipelineData->sets.data()) );

    for (size_t i = 0; i < vkResources.renderTextures.size(); i++)
    {
        VkDescriptorImageInfo computeImgInfo[CL_STORAGE_IMAGES];
        computeImgInfo[CL_RENDER_TEXTURE].imageView = vkResources.renderTextures[i].texView;
        computeImgInfo[CL_RENDER_TEXTURE].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        computeImgInfo[CL_RENDER_TEXTURE].sampler = nullptr;

        computeImgInfo[CL_COMPUTE_TEXTURE].imageView = vkResources.computeTextures[i].texView;
        computeImgInfo[CL_COMPUTE_TEXTURE].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        computeImgInfo[CL_COMPUTE_TEXTURE].sampler = nullptr;

        VkWriteDescriptorSet computeWrite[CL_STORAGE_IMAGES] = {};
        computeWrite[CL_RENDER_TEXTURE].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        computeWrite[CL_RENDER_TEXTURE].dstSet = pipelineData->sets[i];
        computeWrite[CL_RENDER_TEXTURE].dstBinding = CL_RENDER_TEXTURE;
        computeWrite[CL_RENDER_TEXTURE].dstArrayElement = 0;
        computeWrite[CL_RENDER_TEXTURE].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        computeWrite[CL_RENDER_TEXTURE].descriptorCount = 1;
        computeWrite[CL_RENDER_TEXTURE].pImageInfo = &computeImgInfo[CL_RENDER_TEXTURE];

        computeWrite[CL_COMPUTE_TEXTURE].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        computeWrite[CL_COMPUTE_TEXTURE].dstSet = pipelineData->sets[i];
        computeWrite[CL_COMPUTE_TEXTURE].dstBinding = CL_COMPUTE_TEXTURE;
        computeWrite[CL_COMPUTE_TEXTURE].dstArrayElement = 0;
        computeWrite[CL_COMPUTE_TEXTURE].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        computeWrite[CL_COMPUTE_TEXTURE].descriptorCount = 1;
        computeWrite[CL_COMPUTE_TEXTURE].pImageInfo = &computeImgInfo[CL_COMPUTE_TEXTURE];

        vkUpdateDescriptorSets(vkResources.device, CL_STORAGE_IMAGES, computeWrite, 0, nullptr);
    }
}

void Renderer::SetImageSets(
    VulkanPipelineData* pipelineData)
{
    vector<VkDescriptorImageInfo> texInfos(pipelineData->textures.size());

    for (int i = 0; i < texInfos.size(); i++)
    {
        texInfos[i] = {};
        texInfos[i].imageView = pipelineData->textures[i].texView;
        texInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texInfos[i].sampler = pipelineData->sampler;
    }


    vector<VkWriteDescriptorSet> updates(pipelineData->sets.size());
    for (int i = 0; i < updates.size(); i++)
    {
        updates[i] = {};
        updates[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        updates[i].dstSet = pipelineData->sets[i];
        updates[i].dstBinding = GFX_SAMPLED_IMAGE;
        updates[i].dstArrayElement = 0;
        updates[i].descriptorCount = texInfos.size();
        updates[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        updates[i].pImageInfo = texInfos.data();
    }

    vkUpdateDescriptorSets(vkResources.device, updates.size(), updates.data(), 0, nullptr);
    int x = 2;

}

void Renderer::CreateSampler(
    VulkanPipelineData* pipelineData)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vkResources.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    EXIT_ON_VK_ERROR(vkCreateSampler(vkResources.device, &samplerInfo, nullptr, &pipelineData->sampler));
}

void Renderer::CreateGraphicsSets(
    VulkanPipelineData* pipelineData)
{
    pipelineData->maxSets = 6;
    VkDescriptorPoolSize gfxPoolSize[2] = {};
    gfxPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    gfxPoolSize[0].descriptorCount = GFX_ENTRY_COUNT * pipelineData->maxSets; 
    gfxPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gfxPoolSize[1].descriptorCount = pipelineData->texDims.size() * pipelineData->maxSets;

    VkDescriptorPoolCreateInfo gfxPoolDesc = {};
    gfxPoolDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    gfxPoolDesc.maxSets = pipelineData->maxSets;
    gfxPoolDesc.poolSizeCount = 2;
    gfxPoolDesc.pPoolSizes = gfxPoolSize;
    EXIT_ON_VK_ERROR(vkCreateDescriptorPool(vkResources.device, &gfxPoolDesc, nullptr, &pipelineData->descriptorPool));

    pipelineData->sets.resize(pipelineData->maxSets);
    vector<VkDescriptorSetLayout> layouts(pipelineData->maxSets, pipelineData->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pipelineData->descriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    EXIT_ON_VK_ERROR(vkAllocateDescriptorSets(vkResources.device, &allocInfo, pipelineData->sets.data()));
    
}

void Renderer::CreateBasicGraphicsLayout(
    uint8_t textureCount,
    VkDescriptorSetLayout* setLayout, 
    VkPipelineLayout* pipelineLayout)
{
    VkDescriptorSetLayoutBinding bindings[3] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = textureCount;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descSetLayoutInfo = {};
    descSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutInfo.bindingCount = 3;
    descSetLayoutInfo.pBindings = bindings;
    EXIT_ON_VK_ERROR(vkCreateDescriptorSetLayout(vkResources.device, &descSetLayoutInfo, nullptr, setLayout));

    VkPipelineLayoutCreateInfo layoutInfoGraphics = {};
    layoutInfoGraphics.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfoGraphics.setLayoutCount = 1;
    layoutInfoGraphics.pSetLayouts = setLayout;
    EXIT_ON_VK_ERROR(vkCreatePipelineLayout(vkResources.device, &layoutInfoGraphics, nullptr, pipelineLayout));
}

void Renderer::CreateBasicGraphicsVkPipeline(
    VkPipelineLayout pipelineLayout,
    VkPipeline* pipeline,
    uint8_t lightCount,
    uint8_t textureCount)
{
    ShaderOptions opts;
    string strLightCount = to_string(lightCount);
    string strTextureCount = to_string(textureCount);

    opts.AddOption("LIGHT_COUNT", strLightCount.c_str());
    opts.AddOption("TEXTURE_COUNT", strTextureCount.c_str());
    VkShaderModule vertexShader = compiler.CompileShaderFromPath(vkResources.device, nullptr, "shaders/basic.vert", "main", shaderc_vertex_shader, opts);
    VkShaderModule fragmentShader = compiler.CompileShaderFromPath(vkResources.device, nullptr, "shaders/basic.frag", "main", shaderc_fragment_shader, opts);


    VkPipelineShaderStageCreateInfo shaderInfo[2] = {};
    shaderInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfo[0].module = vertexShader;
    shaderInfo[0].pName = "main";

    shaderInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderInfo[1].module = fragmentShader;
    shaderInfo[1].pName = "main";

    VkVertexInputBindingDescription vertBind = {};
    vertBind.binding = 0;
    vertBind.stride = sizeof(CommonVertex);
    vertBind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertAttr[3] = {};
    vertAttr[0].binding = 0;
    vertAttr[0].location = 0;
    vertAttr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertAttr[0].offset = offsetof(CommonVertex, pos);

    vertAttr[1].binding = 0;
    vertAttr[1].location = 1;
    vertAttr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertAttr[1].offset = offsetof(CommonVertex, normal);

    vertAttr[2].binding = 0;
    vertAttr[2].location = 2;
    vertAttr[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertAttr[2].offset = offsetof(CommonVertex, tex);

    VkPipelineVertexInputStateCreateInfo inputStateInfo = {};
    inputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateInfo.vertexBindingDescriptionCount = 1;
    inputStateInfo.pVertexBindingDescriptions = &vertBind;
    inputStateInfo.vertexAttributeDescriptionCount = 3;
    inputStateInfo.pVertexAttributeDescriptions = vertAttr;

    VkPipelineInputAssemblyStateCreateInfo inputAss = {};
    inputAss.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAss.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAss.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo vpInfo = {};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.viewportCount = 1;
    vpInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterInfo = {};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterInfo.depthBiasEnable = VK_FALSE;
    rasterInfo.depthBiasConstantFactor = 0.0f;
    rasterInfo.depthBiasClamp = 0.0f; 
    rasterInfo.depthBiasSlopeFactor = 0.0f; 
    rasterInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; 
    multisampling.pSampleMask = nullptr; 
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;


    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.logicOp = VK_LOGIC_OP_COPY;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &colorBlendAttachment;
    blendInfo.blendConstants[0] = 0.0f;
    blendInfo.blendConstants[1] = 0.0f;
    blendInfo.blendConstants[2] = 0.0f;
    blendInfo.blendConstants[3] = 0.0f;

    VkDynamicState dynamicStates[2] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineDepthStencilStateCreateInfo depthInfo = {};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderInfo;
    pipelineInfo.pVertexInputState = &inputStateInfo;
    pipelineInfo.pInputAssemblyState = &inputAss;
    pipelineInfo.pViewportState = &vpInfo;
    pipelineInfo.pRasterizationState = &rasterInfo;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthInfo;
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = vkResources.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    EXIT_ON_VK_ERROR(vkCreateGraphicsPipelines(vkResources.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline));
 
    vkDestroyShaderModule(vkResources.device, fragmentShader, nullptr);
    vkDestroyShaderModule(vkResources.device, vertexShader, nullptr);
}


