/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanFuncs.h"
#include "PostExecutionOp.h"

#include "../VulkanObjs.h"

#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanCommandPool.h"
#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan//VulkanDevice.h"
#include "../Vulkan/VulkanPhysicalDevice.h"

#include "../Buffer/IBuffers.h"
#include "../VMA/IVMA.h"

#include <Accela/Render/IVulkanCalls.h>

#include <format>

namespace Accela::Render
{

VulkanFuncs::VulkanFuncs(Common::ILogger::Ptr logger,
                         VulkanObjsPtr vulkanObjs)
    : m_logger(std::move(logger))
    , m_vulkanObjs(std::move(vulkanObjs))
{

}

std::optional<VkFormat> VulkanFuncs::ImageDataFormatToVkFormat(const Common::ImageData::PixelFormat& format)
{
    switch (format)
    {
        case Common::ImageData::PixelFormat::RGBA32: return VK_FORMAT_R8G8B8A8_SRGB;
        default: return std::nullopt;
    }
}

VkFormatProperties VulkanFuncs::GetVkFormatProperties(const VkFormat& vkFormat) const
{
    VkFormatProperties vkFormatProperties{};

    m_vulkanObjs->GetCalls()->vkGetPhysicalDeviceFormatProperties(
        m_vulkanObjs->GetPhysicalDevice()->GetVkPhysicalDevice(),
        vkFormat,
        &vkFormatProperties
    );

    return vkFormatProperties;
}

bool VulkanFuncs::QueueSubmit(const std::string& tag,
                              const PostExecutionOpsPtr& postExecutionOps,
                              VkQueue vkQueue,
                              const VulkanCommandPoolPtr& commandPool,
                              const std::function<void(const VulkanCommandBufferPtr&, VkFence)>& func)
{
    const auto commandBufferOpt = commandPool->AllocateCommandBuffer(VulkanCommandPool::CommandBufferType::Primary, "QueueSubmit-" + tag);
    if (!commandBufferOpt.has_value())
    {
        m_logger->Log(Common::LogLevel::Fatal, "QueueSubmit: Failed to create command buffer");
        return false;
    }
    const auto& commandBuffer = commandBufferOpt.value();
    const auto vkCommandBuffer = commandBuffer->GetVkCommandBuffer();

    VkFenceCreateInfo vkFenceCreateInfo{};
    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkFenceCreateInfo.pNext = nullptr;
    vkFenceCreateInfo.flags = 0;

    VkFence vkExecutionFence{VK_NULL_HANDLE};
    m_vulkanObjs->GetCalls()->vkCreateFence(m_vulkanObjs->GetDevice()->GetVkDevice(), &vkFenceCreateInfo, nullptr, &vkExecutionFence);

    commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    func(commandBuffer, vkExecutionFence);
    commandBuffer->End();

    QueueSubmit(tag, vkQueue, {vkCommandBuffer}, WaitOn::None(), SignalOn::None(), vkExecutionFence);

    postExecutionOps->Enqueue(vkExecutionFence, FreeCommandBufferOp(commandPool, commandBuffer));
    postExecutionOps->Enqueue(vkExecutionFence, DeleteFenceOp(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), vkExecutionFence));

    return true;
}

bool VulkanFuncs::QueueSubmit(const std::string& tag,
                              const VkQueue& vkQueue,
                              const std::vector<VkCommandBuffer>& commandBuffers,
                              const WaitOn& waitOn,
                              const SignalOn& signalOn,
                              const std::optional<VkFence>& fence)
{
    VkFence vkFence = VK_NULL_HANDLE;
    if (fence.has_value())
    {
        vkFence = fence.value();
    }

    VkSubmitInfo swapChainRender_submitInfo{};
    swapChainRender_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    swapChainRender_submitInfo.waitSemaphoreCount = waitOn.semaphores.size();
    swapChainRender_submitInfo.pWaitSemaphores = waitOn.semaphores.data();
    swapChainRender_submitInfo.pWaitDstStageMask = waitOn.stageFlags.data();
    swapChainRender_submitInfo.commandBufferCount = commandBuffers.size();
    swapChainRender_submitInfo.pCommandBuffers = commandBuffers.data();
    swapChainRender_submitInfo.signalSemaphoreCount = signalOn.semaphores.size();
    swapChainRender_submitInfo.pSignalSemaphores = signalOn.semaphores.data();

    { QueueSectionLabel queueLabel(m_vulkanObjs->GetCalls(), vkQueue, tag);
        const auto result = m_vulkanObjs->GetCalls()->vkQueueSubmit(vkQueue, 1, &swapChainRender_submitInfo, vkFence);
        if (result != VK_SUCCESS)
        {
            m_logger->Log(Common::LogLevel::Error,
              "QueueSubmit: vkQueueSubmit call failure, result code: {}", (uint32_t)result);
            return false;
        }
    }

    return true;
}

bool VulkanFuncs::TransferImageData(const IBuffersPtr& buffers,
                                    const PostExecutionOpsPtr& postExecutionOps,
                                    VkCommandBuffer vkCommandBuffer,
                                    VkFence vkExecutionFence,
                                    const Common::ImageData::Ptr& imageData,
                                    VkImage vkDestImage,
                                    const uint32_t & mipLevels,
                                    VkPipelineStageFlags vkPipelineUsageFlags,
                                    VkImageLayout vkFinalImageLayout)
{
    //
    // Create a CPU-only staging buffer and fill it with the image's data
    //
    const auto stagingBuffer = buffers->CreateBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        imageData->GetTotalByteSize(),
        std::format("TransferStaging-{}", (uint64_t)vkDestImage)
    );
    if (!stagingBuffer.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "TransferImageData: Failed to create staging buffer for: {}", (uint64_t)vkDestImage);
        return false;
    }

    BufferUpdate stagingUpdate{};
    stagingUpdate.pData = (void*)imageData->GetPixelBytes().data();
    stagingUpdate.updateOffset = 0;
    stagingUpdate.dataByteSize = imageData->GetTotalByteSize();

    if (!buffers->MappedUpdateBuffer(*stagingBuffer, {stagingUpdate}))
    {
        m_logger->Log(Common::LogLevel::Error,
          "TransferImageData: Failed to update staging buffer for: {}", (uint64_t)vkDestImage);
        buffers->DestroyBuffer((*stagingBuffer)->GetBufferId());
        return false;
    }

    //
    // Append commands to copy from the staging buffer to the VkImage
    //

    // Pipeline barrier before the data copy to transition the VkImage into transfer destination optimal format
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = imageData->GetNumLayers();

    VkImageMemoryBarrier imageBarrierToTransfer = {};
    imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrierToTransfer.image = vkDestImage;
    imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrierToTransfer.subresourceRange = range;
    imageBarrierToTransfer.srcAccessMask = 0;
    imageBarrierToTransfer.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

    m_vulkanObjs->GetCalls()->vkCmdPipelineBarrier(
        vkCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageBarrierToTransfer
    );

    // Copy the data from the staging buffer to the VkImage
    VkExtent3D sourceExtent{};
    sourceExtent.width = imageData->GetPixelWidth();
    sourceExtent.height = imageData->GetPixelHeight();
    sourceExtent.depth = 1;

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = imageData->GetNumLayers();
    copyRegion.imageExtent = sourceExtent;

    m_vulkanObjs->GetCalls()->vkCmdCopyBufferToImage(
        vkCommandBuffer,
        (*stagingBuffer)->GetVkBuffer(),
        vkDestImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );

    // Pipeline to barrier reading from the VkImage until the transfer is done, and
    // convert the VkImage from transfer destination optimal format to its final format
    VkImageMemoryBarrier imageBarrierToReadable = {};
    imageBarrierToReadable.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrierToReadable.image = vkDestImage;
    imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrierToReadable.newLayout = vkFinalImageLayout;
    imageBarrierToReadable.subresourceRange = range;
    imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_vulkanObjs->GetCalls()->vkCmdPipelineBarrier(
        vkCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        vkPipelineUsageFlags,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageBarrierToReadable
    );

    //
    // Record a task to clean up the staging buffer once the transfer is complete
    //
    postExecutionOps->Enqueue(vkExecutionFence, BufferDeleteOp(buffers, (*stagingBuffer)->GetBufferId()));

    return true;
}

void VulkanFuncs::GenerateMipMaps(VkCommandBuffer vkCommandBuffer,
                                  const USize& imageSize,
                                  VkImage vkImage,
                                  uint32_t mipLevels,
                                  VkPipelineStageFlags vkPipelineUsageFlags,
                                  VkImageLayout vkFinalImageLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = vkImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    auto mipWidth = static_cast<int32_t>(imageSize.w);
    auto mipHeight = static_cast<int32_t>(imageSize.h);

    //
    // For each mip level, blit from the previous blit mip level to it
    //
    for (uint32_t mipLevel = 1; mipLevel < mipLevels; ++mipLevel)
    {
        //
        // Transfer the previous mip level's layout to transfer source optimal before blitting
        // from it. Also waits for any transfer that was happening to it to finish.
        //
        barrier.subresourceRange.baseMipLevel = mipLevel - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        m_vulkanObjs->GetCalls()->vkCmdPipelineBarrier(
            vkCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        //
        // Blit from the previous mip level to this mip level
        //
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = mipLevel - 1; // Previous mip level
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1; // TODO: Support mipmaping multi layer images?
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = mipLevel; // Current mip level
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1; // TODO: Support mipmaping multi layer images?

        m_vulkanObjs->GetCalls()->vkCmdBlitImage(
            vkCommandBuffer,
            vkImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vkImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_LINEAR
        );

        //
        // Barrier to transfer the previous mip level to the final layout and
        // wait for transfers from it to finish
        //
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = vkFinalImageLayout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        m_vulkanObjs->GetCalls()->vkCmdPipelineBarrier(
            vkCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            vkPipelineUsageFlags,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    //
    // Barrier to transfer the final mip level to the final layout and
    // wait for transfers from it to finish.
    //
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = vkFinalImageLayout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_vulkanObjs->GetCalls()->vkCmdPipelineBarrier(
        vkCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        vkPipelineUsageFlags,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
}

}
