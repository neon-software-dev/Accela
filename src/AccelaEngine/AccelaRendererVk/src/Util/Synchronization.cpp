/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Synchronization.h"

#include "../Buffer/Buffer.h"
#include "../Image/IImages.h"
#include "../Vulkan/VulkanCommandBuffer.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

SemaphoreWait::SemaphoreWait(VkSemaphore _semaphore, VkPipelineStageFlags _stageFlags)
    : semaphore(_semaphore)
    , stageFlags(_stageFlags)
{ }

WaitOn::WaitOn(const std::vector<SemaphoreWait>& _semaphores)
{
    for (const auto& s : _semaphores)
    {
        semaphores.push_back(s.semaphore);
        stageFlags.push_back(s.stageFlags);
    }
}

SignalOn::SignalOn(std::vector<VkSemaphore> _semaphores)
    : semaphores(std::move(_semaphores))
{ }

ImageAccess::ImageAccess(VkImageLayout _requiredInitialLayout,
                         VkImageLayout _finalLayout,
                         BarrierPoint _earliestUsage,
                         BarrierPoint _latestUsage,
                         Layers _layers,
                         Levels _levels,
                         VkImageAspectFlags _vkImageAspect)
    : requiredInitialLayout(_requiredInitialLayout)
    , finalLayout(_finalLayout)
    , earliestUsage(_earliestUsage)
    , latestUsage(_latestUsage)
    , layers(_layers)
    , levels(_levels)
    , vkImageAspect(_vkImageAspect)
{

}

ImageAccess::ImageAccess(BarrierPoint _earliestUsage,
                         BarrierPoint _latestUsage,
                         Layers _layers,
                         Levels _levels,
                         VkImageAspectFlags _vkImageAspect)
    : earliestUsage(_earliestUsage)
    , latestUsage(_latestUsage)
    , layers(_layers)
    , levels(_levels)
    , vkImageAspect(_vkImageAspect)
{

}

void InsertPipelineBarrier_Buffer(const IVulkanCallsPtr& vk,
                                  const VulkanCommandBufferPtr& commandBuffer,
                                  const SourceStage& sourceStage,
                                  const DestStage& destStage,
                                  const BufferMemoryBarrier& memoryBarrier)
{
    VkBufferMemoryBarrier bufferMemoryBarrier{};
    bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferMemoryBarrier.pNext = nullptr;
    bufferMemoryBarrier.srcAccessMask = memoryBarrier.sourceAccess.flags;
    bufferMemoryBarrier.dstAccessMask = memoryBarrier.destAccess.flags;
    bufferMemoryBarrier.buffer = memoryBarrier.buffer->GetVkBuffer();
    bufferMemoryBarrier.offset = memoryBarrier.offset;
    bufferMemoryBarrier.size = memoryBarrier.byteSize;

    vk->vkCmdPipelineBarrier(
        commandBuffer->GetVkCommandBuffer(),
        sourceStage.stage,
        destStage.stage,
        0,
        0,
        nullptr,
        1,
        &bufferMemoryBarrier,
        0,
        nullptr
    );
}

void InsertPipelineBarrier_Image(const IVulkanCallsPtr& vk,
                                 const IImagesPtr& images,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const LoadedImage& loadedImage,
                                 const Layers& layers,
                                 const Levels& levels,
                                 const VkImageAspectFlags& vkImageAspectFlags,
                                 const BarrierPoint& source,
                                 const BarrierPoint& dest,
                                 const ImageTransition& imageTransition)
{
    //
    // Create image barrier
    //
    VkImageSubresourceRange range;
    range.aspectMask = vkImageAspectFlags;
    range.baseMipLevel = levels.baseLevel;
    range.levelCount = levels.levelCount;
    range.baseArrayLayer = layers.startLayer;
    range.layerCount = layers.numLayers;

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.image = loadedImage.allocation.vkImage;
    imageMemoryBarrier.oldLayout = imageTransition.oldLayout;
    imageMemoryBarrier.newLayout = imageTransition.newLayout;
    imageMemoryBarrier.subresourceRange = range;
    imageMemoryBarrier.srcAccessMask = source.access;
    imageMemoryBarrier.dstAccessMask = dest.access;

    vk->vkCmdPipelineBarrier(
        commandBuffer->GetVkCommandBuffer(),
        source.stage,
        dest.stage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageMemoryBarrier
    );

    //
    // Update the internal image state to track the image's layout after the barrier
    //
    images->RecordImageLayout(loadedImage.id, imageTransition.newLayout);
}

}
