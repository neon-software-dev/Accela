/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderTargets.h"

#include "../VulkanObjs.h"
#include "../InternalCommon.h"

#include "../Framebuffer/IFramebuffers.h"
#include "../Texture/ITextures.h"

#include <format>

namespace Accela::Render
{

RenderTargets::RenderTargets(Common::ILogger::Ptr logger,
                             VulkanObjsPtr vulkanObjs,
                             IFramebuffersPtr framebuffers,
                             ITexturesPtr textures,
                             Ids::Ptr ids)
     : m_logger(std::move(logger))
     , m_vulkanObjs(std::move(vulkanObjs))
     , m_framebuffers(std::move(framebuffers))
     , m_textures(std::move(textures))
     , m_ids(std::move(ids))
{

}

Render::TextureView TextureViewForLayerCount(Render::TextureView::Aspect aspect, uint32_t layerCount)
{
    //
    // If we're creating single layer render textures for desktop mode, our view of those textures is as a simple
    // one layer 2D image. If we created multiple layer textures for rendering in VR mode, we view the texture as
    // a texture array over all the texture's layers.
    //
    if (layerCount == 1)
    {
        return Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT, aspect);
    }
    else
    {
        return Render::TextureView::ViewAs2DArray(Render::TextureView::DEFAULT, aspect, Render::TextureView::Layer(0, layerCount));
    }
}

bool RenderTargets::CreateRenderTarget(const RenderTargetId& renderTargetId, const std::string& tag)
{
    if (m_renderTargets.contains(renderTargetId))
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreateRenderTarget: RenderTargetId already exists: {}", renderTargetId.id);
        return false;
    }

    const auto gPassFramebufferId = CreateGPassFramebuffer(tag);
    if (!gPassFramebufferId)
    {
        m_logger->Log(Common::LogLevel::Error,
            "RenderTargets::CreateRenderTarget: Failed to create GPass framebuffer: {}",tag);
        return false;
    }

    const auto blitFramebufferId = CreateBlitFramebuffer(*gPassFramebufferId, tag);
    if (!blitFramebufferId)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreateRenderTarget: Failed to create blit framebuffer: {}",tag);
        return false;
    }

    m_renderTargets.insert({
        renderTargetId,
        RenderTarget(*gPassFramebufferId, *blitFramebufferId, tag)
    });

    return true;
}

std::optional<Render::FrameBufferId> RenderTargets::CreateGPassFramebuffer(const std::string& tag) const
{
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    uint32_t layerCount = 1;

    // If we're presenting to a headset, create two layers for each render target texture, to hold the output for each eye
    if (renderSettings.presentToHeadset)
    {
        layerCount = 2;
    }

    const auto attachmentTextureSampler = TextureSampler(CLAMP_ADDRESS_MODE);

    const auto colorAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::Storage},
            TextureFormat::R32G32B32A32_SFLOAT,
            renderSettings.resolution,
            layerCount,
            false,
            "Color"
        ),
        { TextureView::ViewAs2DArray(
            TextureView::DEFAULT,
            TextureView::Aspect::ASPECT_COLOR_BIT,
            TextureView::Layer(0, layerCount)
        ) },
        attachmentTextureSampler
    };

    const auto positionAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::ColorAttachment, TextureUsage::InputAttachment},
            TextureFormat::R32G32B32A32_SFLOAT,
            renderSettings.resolution,
            layerCount,
            false,
            "Position"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        attachmentTextureSampler
    };

    const auto normalAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::ColorAttachment, TextureUsage::InputAttachment},
            TextureFormat::R32G32B32A32_SFLOAT,
            renderSettings.resolution,
            layerCount,
            false,
            "Normal"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        attachmentTextureSampler
    };

    const auto materialAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::ColorAttachment, TextureUsage::InputAttachment},
            TextureFormat::R32_UINT,
            renderSettings.resolution,
            layerCount,
            false,
            "Material"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        attachmentTextureSampler
    };

    const auto ambientAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::InputAttachment},
            TextureFormat::R8G8B8A8_SRGB,
            renderSettings.resolution,
            layerCount,
            false,
            "Ambient"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        attachmentTextureSampler
    };

    const auto diffuseAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::InputAttachment},
            TextureFormat::R8G8B8A8_SRGB,
            renderSettings.resolution,
            layerCount,
            false,
            "Diffuse"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        attachmentTextureSampler
    };

    const auto specularAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::InputAttachment},
            TextureFormat::R8G8B8A8_SRGB,
            renderSettings.resolution,
            layerCount,
            false,
            "Specular"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        attachmentTextureSampler
    };

    const auto depthAttachmentTexture = TextureDefinition {
        Texture::EmptyDepth(
            Render::TextureId::Invalid(),
            {TextureUsage::DepthStencilAttachment, TextureUsage::Sampled},
            renderSettings.resolution,
            layerCount,
            false,
            "Depth"
        ),
        { TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_DEPTH_BIT, layerCount) },
        attachmentTextureSampler
    };

    //
    // Create the GPass Framebuffer
    //

    const auto gPassFramebufferId = m_ids->frameBufferIds.GetId();

    const bool result = m_framebuffers->CreateFramebuffer(
        gPassFramebufferId,
        m_vulkanObjs->GetGPassRenderPass(),
        {
            {colorAttachmentTexture, TextureView::DEFAULT},
            {positionAttachmentTexture,TextureView::DEFAULT},
            {normalAttachmentTexture, TextureView::DEFAULT},
            {materialAttachmentTexture,TextureView::DEFAULT},
            {ambientAttachmentTexture, TextureView::DEFAULT},
            {diffuseAttachmentTexture,TextureView::DEFAULT},
            {specularAttachmentTexture,TextureView::DEFAULT},
            {depthAttachmentTexture, TextureView::DEFAULT}
        },
        m_vulkanObjs->GetRenderSettings().resolution,
        1,
        std::format("RenderTarget-{}", tag)
    );

    if (!result)
    {
        m_ids->frameBufferIds.ReturnId(gPassFramebufferId);

        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreateRenderTarget: Failed to create gPass framebuffer: {}", tag);

        return std::nullopt;
    }

    return gPassFramebufferId;
}

std::optional<FrameBufferId> RenderTargets::CreateBlitFramebuffer(FrameBufferId gPassFramebufferId, const std::string& tag) const
{
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    uint32_t layerCount = 1;

    // If we're presenting to a headset, create two layers for each render target texture, to hold the output for each eye
    if (renderSettings.presentToHeadset)
    {
        layerCount = 2;
    }

    const auto gPassFramebufferObjs = m_framebuffers->GetFramebufferObjs(gPassFramebufferId);
    if (!gPassFramebufferObjs) { return std::nullopt; }

    const auto blitFramebufferId = m_ids->frameBufferIds.GetId();

    // Note that this framebuffer is a non-owning framebuffer that just references textures that were created/owned
    // by the gpass framebuffer
    const auto result = m_framebuffers->CreateFramebuffer(
        blitFramebufferId,
        m_vulkanObjs->GetBlitRenderPass(),
        {
            std::make_pair(gPassFramebufferObjs->GetAttachmentTexture(Offscreen_Attachment_Color)->first.textureId, TextureView::DEFAULT),
            std::make_pair(gPassFramebufferObjs->GetAttachmentTexture(Offscreen_Attachment_Depth)->first.textureId, TextureView::DEFAULT)
        },
        m_vulkanObjs->GetRenderSettings().resolution,
        layerCount,
        tag
    );

    if (!result)
    {
        m_ids->frameBufferIds.ReturnId(blitFramebufferId);

        m_logger->Log(Common::LogLevel::Error,
              "RenderTargets::CreateRenderTarget: Failed to create blit framebuffer: {}", tag);

        return std::nullopt;
    }

    return blitFramebufferId;
}

void RenderTargets::DestroyRenderTarget(const RenderTargetId& renderTargetId, bool destroyImmediately)
{
    const auto it = m_renderTargets.find(renderTargetId);
    if (it == m_renderTargets.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Debug, "RenderTargets: Destroying render target: {}", renderTargetId.id);

    m_framebuffers->DestroyFramebuffer(it->second.blitFramebuffer, destroyImmediately);
    m_framebuffers->DestroyFramebuffer(it->second.gPassFramebuffer, destroyImmediately);

    m_renderTargets.erase(it);
}

std::optional<RenderTarget> RenderTargets::GetRenderTarget(const RenderTargetId& renderTargetId) const
{
    const auto it = m_renderTargets.find(renderTargetId);
    if (it == m_renderTargets.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

bool RenderTargets::OnRenderSettingsChanged(const RenderSettings&)
{
    const auto renderTargetsCopy = m_renderTargets;

    Destroy();

    bool allSuccessful = true;

    for (const auto& renderTarget : renderTargetsCopy)
    {
        if (!CreateRenderTarget(renderTarget.first, renderTarget.second.tag))
        {
            m_logger->Log(Common::LogLevel::Error,
              "RenderTargets::OnRenderSettingsChanged: Failed to recreate render target: {}", renderTarget.second.tag);

            allSuccessful = false;
        }
    }

    return allSuccessful;
}

void RenderTargets::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "RenderTargets: Destroying");

    while (!m_renderTargets.empty())
    {
        DestroyRenderTarget(m_renderTargets.cbegin()->first, true);
    }
}

}
