/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderState.h"

#include "Image/IImages.h"

namespace Accela::Render
{

RenderState::RenderState(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IImagesPtr images)
    : m_logger(std::move(logger))
    , m_vulkanCalls(std::move(vulkanCalls))
    , m_images(std::move(images))
{

}

void RenderState::PrepareOperation(const VulkanCommandBufferPtr& commandBuffer, const RenderOperation& renderOperation)
{
    for (const auto& imageAccess : renderOperation.GetImageAccesses())
    {
        const auto loadedImage = m_images->GetImage(imageAccess.first);
        if (!loadedImage)
        {
            m_logger->Log(Common::LogLevel::Error, "RenderState::PrepareOperation: No such image: {}", imageAccess.first.id);
            continue;
        }

        PrepareImageAccess(commandBuffer, *loadedImage, imageAccess.second);
    }
}

void RenderState::PrepareImageAccess(const VulkanCommandBufferPtr& commandBuffer, const LoadedImage& loadedImage, const ImageAccess& imageAccess)
{
    if (!m_imageStates.contains(loadedImage.id))
    {
        m_imageStates.insert({loadedImage.id, ImageState{}});
    }

    PrepareImageAccess(commandBuffer, loadedImage, imageAccess, m_imageStates.at(loadedImage.id));
}

void RenderState::PrepareImageAccess(const VulkanCommandBufferPtr& commandBuffer,
                                     const LoadedImage& loadedImage,
                                     const ImageAccess& imageAccess,
                                     RenderState::ImageState& currentState)
{
    const bool needsLayoutTransition =
        (loadedImage.vkImageLayout != imageAccess.requiredInitialLayout) &&
        (imageAccess.requiredInitialLayout != VK_IMAGE_LAYOUT_UNDEFINED);

    const bool needsSynchronization = currentState.currentAccess.has_value();

    if (!needsLayoutTransition && !needsSynchronization)
    {
        currentState.currentAccess = imageAccess;

        return;
    }

    BarrierPoint currentUsage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

    if (currentState.currentAccess)
    {
        currentUsage = currentState.currentAccess->latestUsage;
    }

    VkImageLayout vkNewLayout = imageAccess.finalLayout;

    // If the new work requires an undefined layout, don't perform any
    // layout transition; just keep the layout at what it currently is
    if (vkNewLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        vkNewLayout = loadedImage.vkImageLayout;
    }

    //
    // Insert RenderTexture Barrier
    //
    InsertPipelineBarrier_Image(
        m_vulkanCalls,
        m_images,
        commandBuffer,
        loadedImage,
        imageAccess.layers,
        imageAccess.levels,
        imageAccess.vkImageAspect,
        currentUsage,
        imageAccess.earliestUsage,
        ImageTransition(loadedImage.vkImageLayout, vkNewLayout)
    );

    //
    // Update RenderTexture State
    //
    currentState.currentAccess = imageAccess;
}

void RenderState::Destroy()
{
    m_imageStates.clear();
}

}
