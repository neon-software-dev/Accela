/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderOperation.h"

#include "Vulkan/VulkanRenderPass.h"

namespace Accela::Render
{

RenderOperation::RenderOperation(std::unordered_map<VkImage, ImageAccess> _imageAccesses)
    : m_imageAccesses(std::move(_imageAccesses))
{

}

std::optional<RenderOperation> RenderOperation::FromRenderPass(const FramebufferObjs& frameBufferObjs, const VulkanRenderPassPtr& renderPass)
{
    std::unordered_map<VkImage, ImageAccess> imageAccesses;

    const auto attachmentInitialLayouts = renderPass->GetAttachmentInitialLayouts();
    const auto attachmentFinalLayouts = renderPass->GetAttachmentFinalLayouts();

    const auto attachmentTextures = frameBufferObjs.GetAttachmentTextures();

    if (!attachmentTextures)
    {
        return std::nullopt;
    }

    if (attachmentInitialLayouts.size() != attachmentTextures->size())
    {
        return std::nullopt;
    }

    for (unsigned int attachmentIndex = 0; attachmentIndex < attachmentTextures->size(); ++attachmentIndex)
    {
        const auto attachmentVkImage = attachmentTextures->at(attachmentIndex).first.allocation.vkImage;

        const auto imageAccessIt = imageAccesses.find(attachmentVkImage);
        if (imageAccessIt != imageAccesses.cend())
        {
            return std::nullopt;
        }

        const auto attachmentImageAccess = renderPass->GetAttachmentImageAccess(attachmentIndex);
        if (!attachmentImageAccess)
        {
            return std::nullopt;
        }

        imageAccesses.insert({attachmentVkImage, *attachmentImageAccess});
    }

    return RenderOperation(imageAccesses);
}

}
