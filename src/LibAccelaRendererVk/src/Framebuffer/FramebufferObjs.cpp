/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FramebufferObjs.h"

#include "../VulkanObjs.h"

#include "../Texture/ITextures.h"

#include "../Vulkan/VulkanFramebuffer.h"

#include <Accela/Common/Assert.h>

#include <format>

namespace Accela::Render
{

FramebufferObjs::FramebufferObjs(Common::ILogger::Ptr logger, Ids::Ptr ids, VulkanObjsPtr vulkanObjs, ITexturesPtr textures)
    : m_logger(std::move(logger))
    , m_ids(std::move(ids))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_textures(std::move(textures))
{

}

bool FramebufferObjs::CreateOwning(const VulkanRenderPassPtr& renderPass,
                                   const std::vector<std::pair<TextureDefinition, std::string>>& attachments,
                                   const USize& size,
                                   const uint32_t& layers,
                                   const std::string& tag)
{
    m_logger->Log(Common::LogLevel::Info, "FramebufferObjs::CreateOwning: Creating framebuffer objects for {}", tag);

    //
    // Create textures as requested to supply the framebuffer's attachments
    //
    std::vector<std::pair<TextureId, std::string>> textureViews;

    for (auto& attachment : attachments)
    {
        auto texture = attachment.first.texture;

        Assert(!texture.id.IsValid(), m_logger, "FramebufferObjs::CreateOwning: Texture id was already valid");

        texture.id = m_ids->textureIds.GetId();

        if (!m_textures->CreateTextureEmpty(texture, attachment.first.textureViews, attachment.first.textureSampler))
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to create texture");

            m_ids->textureIds.ReturnId(texture.id);

            for (const auto& it : textureViews)
            {
                m_textures->DestroyTexture(it.first, false);
            }
            return false;
        }

        textureViews.emplace_back(texture.id, attachment.second);
    }

    //
    // Create a framebuffer that references the texture image views
    //
    std::vector<VkImageView> vkImageViews;

    for (const auto& textureView : textureViews)
    {
        const auto loadedTextureOpt = m_textures->GetTexture(textureView.first);
        if (!loadedTextureOpt)
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to get created texture");
            for (const auto& it : textureViews)
            {
                m_textures->DestroyTexture(it.first, true);
            }
            return false;
        }

        vkImageViews.push_back(loadedTextureOpt->vkImageViews.at(textureView.second));
    }

    const auto framebuffer = std::make_shared<VulkanFramebuffer>(m_logger, m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice());
    if (!framebuffer->Create(renderPass, vkImageViews, size, layers, tag))
    {
        m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: Failed to create framebuffer");
        for (const auto& it : textureViews)
        {
            m_textures->DestroyTexture(it.first, true);
        }
        return false;
    }

    //
    // Update internal state
    //
    m_ownsAttachments = true;
    m_attachmentTextureViews = textureViews;
    m_framebuffer = framebuffer;

    return true;
}

bool FramebufferObjs::CreateFromExisting(const VulkanRenderPassPtr& renderPass,
                                         const std::vector<std::pair<TextureId, std::string>>& attachmentTextureViews,
                                         const USize& size,
                                         const uint32_t& layers,
                                         const std::string& tag)
{
    m_logger->Log(Common::LogLevel::Info,
      "FramebufferObjs: Creating framebuffer from objects for {}, resolution: {}x{}", tag, size.w, size.h);

    //
    // Create a framebuffer that references the textures
    //
    std::vector<VkImageView> attachments;

    for (const auto& it : attachmentTextureViews)
    {
        const auto loadedTextureOpt = m_textures->GetTexture(it.first);
        if (!loadedTextureOpt)
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: No such attachment texture exists: {}", it.first.id);
            return false;
        }

        if (!loadedTextureOpt->vkImageViews.contains(it.second))
        {
            m_logger->Log(Common::LogLevel::Error, "Framebuffer: Create: No such attachment texture view exists: {}", it.second);
            return false;
        }

        attachments.push_back(loadedTextureOpt->vkImageViews.at(it.second));
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
    m_attachmentTextureViews = attachmentTextureViews;
    m_framebuffer = framebuffer;

    return true;
}

bool FramebufferObjs::CreateFromExistingDefaultViews(const VulkanRenderPassPtr& renderPass,
                                                     const std::vector<TextureId>& attachmentTextures,
                                                     const USize& size,
                                                     const uint32_t& layers,
                                                     const std::string& tag)
{
    std::vector<std::pair<TextureId, std::string>> attachmentTextureViews;

    for (const auto& textureId : attachmentTextures)
    {
        attachmentTextureViews.emplace_back(textureId, TextureView::DEFAULT);
    }

    return CreateFromExisting(renderPass, attachmentTextureViews, size, layers, tag);
}

void FramebufferObjs::Destroy()
{
    if (m_ownsAttachments)
    {
        for (const auto& it : m_attachmentTextureViews)
        {
            m_textures->DestroyTexture(it.first, true);
        }
        m_attachmentTextureViews.clear();
    }

    if (m_framebuffer != nullptr)
    {
        m_framebuffer->Destroy();
        m_framebuffer = nullptr;
    }

    m_ownsAttachments = false;
}

std::optional<std::vector<std::pair<LoadedTexture, std::string>>> FramebufferObjs::GetAttachmentTextures() const
{
    std::vector<std::pair<LoadedTexture, std::string>> results;

    for (unsigned int x = 0; x < m_attachmentTextureViews.size(); ++x)
    {
        const auto textureView = GetAttachmentTexture(x);
        if (!textureView)
        {
            return std::nullopt;
        }

        results.push_back(*textureView);
    }

    return results;
}

std::optional<std::pair<LoadedTexture, std::string>> FramebufferObjs::GetAttachmentTexture(uint8_t attachmentIndex) const
{
    if (attachmentIndex >= m_attachmentTextureViews.size())
    {
        return std::nullopt;
    }

    const auto attachmentTextureView = m_attachmentTextureViews.at(attachmentIndex);

    const auto loadedTexture = m_textures->GetTexture(attachmentTextureView.first);
    if (!loadedTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
          "GetAttachmentTexture: No such texture exits: {}", attachmentTextureView.first.id);
        return std::nullopt;
    }

    return std::make_pair(*loadedTexture, attachmentTextureView.second);
}

}
