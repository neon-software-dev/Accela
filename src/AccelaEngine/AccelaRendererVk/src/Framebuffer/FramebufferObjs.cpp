/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FramebufferObjs.h"

#include "../VulkanObjs.h"

#include "../Image/IImages.h"

#include "../Vulkan/VulkanFramebuffer.h"

#include <Accela/Common/Assert.h>

#include <format>

namespace Accela::Render
{

FramebufferObjs::FramebufferObjs(Common::ILogger::Ptr logger, Ids::Ptr ids, VulkanObjsPtr vulkanObjs, IImagesPtr images)
    : m_logger(std::move(logger))
    , m_ids(std::move(ids))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_images(std::move(images))
{

}

bool FramebufferObjs::CreateOwning(const VulkanRenderPassPtr& renderPass,
                                   const std::vector<std::pair<ImageDefinition, ImageViewName>>& attachments,
                                   const USize& size,
                                   const uint32_t& layers,
                                   const std::string& tag)
{
    m_logger->Log(Common::LogLevel::Info, "FramebufferObjs::CreateOwning: Creating framebuffer objects for {}", tag);

    //
    // Create images as requested to supply the framebuffer's attachments
    //
    std::vector<std::pair<ImageId, std::string>> imageViews;

    for (auto& attachment : attachments)
    {
        auto imageDefinition = attachment.first;

        const auto imageIdExpect = m_images->CreateEmptyImage(imageDefinition);

        if (!imageIdExpect)
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to create image");

            for (const auto& it : imageViews)
            {
                m_images->DestroyImage(it.first, false);
            }
            return false;
        }

        imageViews.emplace_back(*imageIdExpect, attachment.second);
    }

    //
    // Create a framebuffer that references the image views
    //
    std::vector<VkImageView> vkImageViews;

    for (const auto& imageView : imageViews)
    {
        const auto loadedImageOpt = m_images->GetImage(imageView.first);
        if (!loadedImageOpt)
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to get created image");
            for (const auto& it : imageViews)
            {
                m_images->DestroyImage(it.first, false);
            }
            return false;
        }

        vkImageViews.push_back(loadedImageOpt->vkImageViews.at(imageView.second));
    }

    const auto framebuffer = std::make_shared<VulkanFramebuffer>(m_logger, m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice());
    if (!framebuffer->Create(renderPass, vkImageViews, size, layers, tag))
    {
        m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to create framebuffer");
        for (const auto& it : imageViews)
        {
            m_images->DestroyImage(it.first, false);
        }
        return false;
    }

    //
    // Update internal state
    //
    m_ownsAttachments = true;
    m_attachmentImageViews = imageViews;
    m_framebuffer = framebuffer;

    return true;
}

bool FramebufferObjs::CreateFromExisting(const VulkanRenderPassPtr& renderPass,
                                         const std::vector<std::pair<ImageId, ImageViewName>>& attachmentImageViews,
                                         const USize& size,
                                         const uint32_t& layers,
                                         const std::string& tag)
{
    m_logger->Log(Common::LogLevel::Info,
      "FramebufferObjs: Creating framebuffer from objects for {}, resolution: {}x{}", tag, size.w, size.h);

    //
    // Create a framebuffer that references the images
    //
    std::vector<VkImageView> attachments;

    for (const auto& it : attachmentImageViews)
    {
        const auto loadedImageOpt = m_images->GetImage(it.first);
        if (!loadedImageOpt)
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: No such attachment image exists: {}", it.first.id);
            return false;
        }

        if (!loadedImageOpt->vkImageViews.contains(it.second))
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: No such attachment image view exists: {}", it.second);
            return false;
        }

        attachments.push_back(loadedImageOpt->vkImageViews.at(it.second));
    }

    const auto framebuffer = std::make_shared<VulkanFramebuffer>(m_logger, m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice());
    if (!framebuffer->Create(renderPass, attachments, size, layers, tag))
    {
        m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to create framebuffer");
        return false;
    }

    //
    // Update internal state
    //
    m_ownsAttachments = false;
    m_attachmentImageViews = attachmentImageViews;
    m_framebuffer = framebuffer;

    return true;
}

bool FramebufferObjs::CreateFromExistingDefaultViews(const VulkanRenderPassPtr& renderPass,
                                                     const std::vector<ImageId>& attachmentImages,
                                                     const USize& size,
                                                     const uint32_t& layers,
                                                     const std::string& tag)
{
    std::vector<std::pair<ImageId, std::string>> attachmentImageViews;

    for (const auto& imageId : attachmentImages)
    {
        attachmentImageViews.emplace_back(imageId, ImageView::DEFAULT());
    }

    return CreateFromExisting(renderPass, attachmentImageViews, size, layers, tag);
}

void FramebufferObjs::Destroy()
{
    if (m_ownsAttachments)
    {
        for (const auto& it : m_attachmentImageViews)
        {
            m_images->DestroyImage(it.first, true);
        }
        m_attachmentImageViews.clear();
    }

    if (m_framebuffer != nullptr)
    {
        m_framebuffer->Destroy();
        m_framebuffer = nullptr;
    }

    m_ownsAttachments = false;
}

std::optional<std::vector<std::pair<LoadedImage, ImageViewName>>> FramebufferObjs::GetAttachmentImages() const
{
    std::vector<std::pair<LoadedImage, std::string>> results;

    for (unsigned int x = 0; x < m_attachmentImageViews.size(); ++x)
    {
        const auto imageView = GetAttachmentImage(x);
        if (!imageView)
        {
            return std::nullopt;
        }

        results.push_back(*imageView);
    }

    return results;
}

std::optional<std::pair<LoadedImage, ImageViewName>> FramebufferObjs::GetAttachmentImage(uint8_t attachmentIndex) const
{
    if (attachmentIndex >= m_attachmentImageViews.size())
    {
        return std::nullopt;
    }

    const auto attachmentImageView = m_attachmentImageViews.at(attachmentIndex);

    const auto loadedImage = m_images->GetImage(attachmentImageView.first);
    if (!loadedImage)
    {
        m_logger->Log(Common::LogLevel::Error,
          "GetAttachmentImage: No such image exits: {}", attachmentImageView.first.id);
        return std::nullopt;
    }

    return std::make_pair(*loadedImage, attachmentImageView.second);
}

}
