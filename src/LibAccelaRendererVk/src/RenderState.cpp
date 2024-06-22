/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderState.h"

namespace Accela::Render
{

RenderState::RenderState(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls)
    : m_logger(std::move(logger))
    , m_vulkanCalls(std::move(vulkanCalls))
{

}

void RenderState::PrepareOperation(const VulkanCommandBufferPtr& commandBuffer, const RenderOperation& renderOperation)
{
    for (const auto& imageAccess : renderOperation.GetImageAccesses())
    {
        PrepareImageAccess(commandBuffer, imageAccess.first, imageAccess.second);
    }
}

void RenderState::PrepareImageAccess(const VulkanCommandBufferPtr& commandBuffer, VkImage vkImage, const ImageAccess& imageAccess)
{
    if (!m_imageStates.contains(vkImage))
    {
        m_imageStates.insert({vkImage, ImageState{}});
    }

    PrepareImageAccess(commandBuffer, vkImage, imageAccess, m_imageStates.at(vkImage));
}

void RenderState::PrepareImageAccess(const VulkanCommandBufferPtr& commandBuffer,
                                     VkImage vkImage,
                                     const ImageAccess& imageAccess,
                                     RenderState::ImageState& currentState)
{
    const bool needsLayoutTransition =
        (currentState.currentLayout != imageAccess.requiredInitialLayout) &&
        (imageAccess.requiredInitialLayout != VK_IMAGE_LAYOUT_UNDEFINED);

    const bool needsSynchronization = currentState.currentAccess.has_value();

    if (!needsLayoutTransition && !needsSynchronization)
    {
        currentState.currentLayout = imageAccess.finalLayout;
        currentState.currentAccess = imageAccess;

        return;
    }

    BarrierPoint currentUsage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

    if (currentState.currentAccess)
    {
        currentUsage = currentState.currentAccess->latestUsage;
    }

    VkImageLayout vkNewLayout = imageAccess.requiredInitialLayout;

    // If the new work requires an undefined layout, don't perform any
    // layout transition; just keep the layout at what it currently is
    if (vkNewLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        vkNewLayout = currentState.currentLayout;
    }

    //
    // Insert Image Barrier
    //
    InsertPipelineBarrier_Image(
        m_vulkanCalls,
        commandBuffer,
        vkImage,
        imageAccess.layers,
        imageAccess.levels,
        imageAccess.vkImageAspect,
        currentUsage,
        imageAccess.earliestUsage,
        ImageTransition(currentState.currentLayout, vkNewLayout)
    );

    //
    // Update Image State
    //
    currentState.currentLayout = imageAccess.finalLayout;
    currentState.currentAccess = imageAccess;
}

void RenderState::Destroy()
{
    m_imageStates.clear();
}

}
