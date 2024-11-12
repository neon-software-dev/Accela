/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderOperation.h"

#include "Vulkan/VulkanRenderPass.h"

namespace Accela::Render
{

RenderOperation::RenderOperation(std::unordered_map<ImageId, ImageAccess> _imageAccesses)
    : m_imageAccesses(std::move(_imageAccesses))
{

}

std::optional<RenderOperation> RenderOperation::FromRenderPass(const FramebufferObjs& frameBufferObjs, const VulkanRenderPassPtr& renderPass)
{
    std::unordered_map<ImageId, ImageAccess> imageAccesses;

    const auto attachmentInitialLayouts = renderPass->GetAttachmentInitialLayouts();
    const auto attachmentFinalLayouts = renderPass->GetAttachmentFinalLayouts();

    const auto attachmentImages = frameBufferObjs.GetAttachmentImages();

    if (!attachmentImages)
    {
        return std::nullopt;
    }

    if (attachmentInitialLayouts.size() != attachmentImages->size())
    {
        return std::nullopt;
    }

    for (unsigned int attachmentIndex = 0; attachmentIndex < attachmentImages->size(); ++attachmentIndex)
    {
        const auto& attachmentLoadedImage = attachmentImages->at(attachmentIndex).first;

        const auto imageAccessIt = imageAccesses.find(attachmentLoadedImage.id);
        if (imageAccessIt != imageAccesses.cend())
        {
            return std::nullopt;
        }

        const auto attachmentImageAccess = renderPass->GetAttachmentImageAccess(attachmentIndex);
        if (!attachmentImageAccess)
        {
            return std::nullopt;
        }

        imageAccesses.insert({attachmentLoadedImage.id, *attachmentImageAccess});
    }

    return RenderOperation(imageAccesses);
}

}
