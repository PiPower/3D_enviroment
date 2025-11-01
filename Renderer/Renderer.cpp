#include "Renderer.hpp"
#include "VulkanResourcesInternal.hpp"
#include "errors.hpp"
using namespace std;


enum class ComputeLayout
{
    RenderTexture = 0,
    ComputeTexture,
    StorageImageCount
};

static constexpr uint32_t CL_RENDER_TEXTURE = static_cast<uint32_t>(ComputeLayout::RenderTexture);
static constexpr uint32_t CL_COMPUTE_TEXTURE = static_cast<uint32_t>(ComputeLayout::ComputeTexture);
static constexpr uint32_t CL_STORAGE_IMAGES = static_cast<uint32_t>(ComputeLayout::StorageImageCount);

Renderer::Renderer(
    HINSTANCE hinstance,
    HWND hwnd)
	:
	windowHwnd(hwnd), compiler()
{
	createVulkanResources(&vkResources, hinstance, windowHwnd);
    SetControllingStructs();
    SetupComputeLayout();
    SetupComputePipeline();
    SetupDescriptorSets();
}

void Renderer::BeginRendering()
{
    EXIT_ON_VK_ERROR(vkWaitForFences(vkResources.device, 1, &vkResources.gfxQueueFinished, VK_TRUE, UINT64_MAX));
    EXIT_ON_VK_ERROR(vkResetFences(vkResources.device, 1, &vkResources.gfxQueueFinished));
    EXIT_ON_VK_ERROR(vkAcquireNextImageKHR(vkResources.device, vkResources.swapchain, UINT64_MAX, vkResources.imgReady, VK_NULL_HANDLE, &imageIndex));
}

void Renderer::Render()
{
    VkCommandBufferBeginInfo cmdBuffInfo = {};
    cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    EXIT_ON_VK_ERROR(vkResetCommandBuffer(vkResources.cmdBuffer, 0));
    EXIT_ON_VK_ERROR(vkBeginCommandBuffer(vkResources.cmdBuffer, &cmdBuffInfo));

    vkCmdBeginRenderPass(vkResources.cmdBuffer, &renderPassInfos[imageIndex], VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(vkResources.cmdBuffer);


    vkCmdPipelineBarrier(vkResources.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                                                                                                     0, 0, nullptr, 0, nullptr, 0, nullptr);

    vkCmdBindPipeline(vkResources.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
    vkCmdBindDescriptorSets(vkResources.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.sets[imageIndex], 0, nullptr);

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

}

void Renderer::Present()
{
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &vkResources.swapchain;
    info.pImageIndices = &imageIndex;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vkResources.renderingFinished;
    EXIT_ON_VK_ERROR(vkQueuePresentKHR(vkResources.presentationQueue, &info));

}

void Renderer::SetControllingStructs()
{
    static 	VkClearValue clearColor[2];
    clearColor[1].color = { {0.2f, 0.8f, 0.2f, 1.0f} };
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

void Renderer::SetupComputePipeline()
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
    computeInfo.layout = compute.pipelineLayout;
    computeInfo.stage = shaderInfo;
    computeInfo.basePipelineHandle = VK_NULL_HANDLE;
    computeInfo.basePipelineIndex = -1;

    EXIT_ON_VK_ERROR(vkCreateComputePipelines(vkResources.device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &compute.pipeline));

    vkDestroyShaderModule(vkResources.device, computeShader, nullptr);
}

void Renderer::SetupComputeLayout()
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

    EXIT_ON_VK_ERROR(vkCreateDescriptorSetLayout(vkResources.device, &setInfo, nullptr, &compute.descriptorSetLayout));

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &compute.descriptorSetLayout;

    EXIT_ON_VK_ERROR(vkCreatePipelineLayout(vkResources.device, &layoutInfo, nullptr, &compute.pipelineLayout));
}

void Renderer::SetupDescriptorSets()
{

    compute.sets.resize(vkResources.renderTextures.size());

    VkDescriptorPoolSize poolSize[1] = {};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSize[0].descriptorCount = CL_STORAGE_IMAGES * vkResources.renderTextures.size();

    VkDescriptorPoolCreateInfo poolDesc = {};
    poolDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolDesc.maxSets = vkResources.renderTextures.size();
    poolDesc.poolSizeCount = 1;
    poolDesc.pPoolSizes = poolSize;
    EXIT_ON_VK_ERROR(vkCreateDescriptorPool(vkResources.device, &poolDesc, nullptr, &pipelinesPool));

    vector<VkDescriptorSetLayout> layouts(vkResources.renderTextures.size(), compute.descriptorSetLayout);
    vector<VkDescriptorSet> sets(vkResources.renderTextures.size());
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pipelinesPool;
    allocInfo.descriptorSetCount = vkResources.renderTextures.size();
    allocInfo.pSetLayouts = layouts.data();

    EXIT_ON_VK_ERROR(vkAllocateDescriptorSets(vkResources.device, &allocInfo, sets.data()));
    copy(sets.begin(), sets.begin() + vkResources.renderTextures.size(), compute.sets.begin());

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
        computeWrite[CL_RENDER_TEXTURE].dstSet = compute.sets[i];
        computeWrite[CL_RENDER_TEXTURE].dstBinding = CL_RENDER_TEXTURE;
        computeWrite[CL_RENDER_TEXTURE].dstArrayElement = 0;
        computeWrite[CL_RENDER_TEXTURE].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        computeWrite[CL_RENDER_TEXTURE].descriptorCount = 1;
        computeWrite[CL_RENDER_TEXTURE].pImageInfo = &computeImgInfo[CL_RENDER_TEXTURE];

        computeWrite[CL_COMPUTE_TEXTURE].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        computeWrite[CL_COMPUTE_TEXTURE].dstSet = compute.sets[i];
        computeWrite[CL_COMPUTE_TEXTURE].dstBinding = CL_COMPUTE_TEXTURE;
        computeWrite[CL_COMPUTE_TEXTURE].dstArrayElement = 0;
        computeWrite[CL_COMPUTE_TEXTURE].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        computeWrite[CL_COMPUTE_TEXTURE].descriptorCount = 1;
        computeWrite[CL_COMPUTE_TEXTURE].pImageInfo = &computeImgInfo[CL_COMPUTE_TEXTURE];

        vkUpdateDescriptorSets(vkResources.device, CL_STORAGE_IMAGES, computeWrite, 0, nullptr);
    }
}


