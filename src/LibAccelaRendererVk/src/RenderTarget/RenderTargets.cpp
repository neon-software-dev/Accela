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
#include "../Util/VulkanFuncs.h"
#include "../Vulkan/VulkanDevice.h"

#include <format>

namespace Accela::Render
{

RenderTargets::RenderTargets(Common::ILogger::Ptr logger,
                             VulkanObjsPtr vulkanObjs,
                             PostExecutionOpsPtr postExecutionOps,
                             IFramebuffersPtr framebuffers,
                             ITexturesPtr textures,
                             Ids::Ptr ids)
     : m_logger(std::move(logger))
     , m_vulkanObjs(std::move(vulkanObjs))
     , m_postExecutionOps(std::move(postExecutionOps))
     , m_framebuffers(std::move(framebuffers))
     , m_textures(std::move(textures))
     , m_ids(std::move(ids))
{

}

TextureView TextureViewForLayerCount(TextureView::Aspect aspect, uint32_t layerCount)
{
    //
    // If we're creating single layer render textures for desktop mode, our view of those textures is as a simple
    // one layer 2D image. If we created multiple layer textures for rendering in VR mode, we view the texture as
    // a texture array over all the texture's layers.
    //
    if (layerCount == 1)
    {
        return TextureView::ViewAs2D(TextureView::DEFAULT, aspect);
    }
    else
    {
        return TextureView::ViewAs2DArray(TextureView::DEFAULT, aspect, TextureView::Layer(0, layerCount));
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

    const auto screenFramebufferId = CreateScreenFramebuffer(tag);
    if (!screenFramebufferId)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreateRenderTarget: Failed to create screen framebuffer: {}",tag);
        return false;
    }

    const auto postProcessOutputTexture = CreatePostProcessOutputTexture(tag);
    if (!postProcessOutputTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
              "RenderTargets::CreateRenderTarget: Failed to create post-process output texture: {}",tag);
        return false;
    }

    m_renderTargets.insert({
        renderTargetId,
        RenderTarget(*gPassFramebufferId, *screenFramebufferId, *postProcessOutputTexture, tag)
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

    const auto defaultTextureSampler = TextureSampler(TextureSampler::DEFAULT, CLAMP_ADDRESS_MODE);

    auto nearestTextureSampler = TextureSampler(TextureSampler::NEAREST, CLAMP_ADDRESS_MODE);
    nearestTextureSampler.minFilter = SamplerFilterMode::Nearest;
    nearestTextureSampler.magFilter = SamplerFilterMode::Nearest;

    const auto colorAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::Storage},
            TextureFormat::R16G16B16A16_SFLOAT,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Color-{}", tag)
        ),
        {
            TextureView::ViewAs2DArray(
            TextureView::DEFAULT,
            TextureView::Aspect::ASPECT_COLOR_BIT,
            TextureView::Layer(0, layerCount))
        },
        { defaultTextureSampler, nearestTextureSampler }
    };

    const auto positionAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::ColorAttachment, TextureUsage::InputAttachment},
            TextureFormat::R32G32B32A32_SFLOAT,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Position-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        { defaultTextureSampler }
    };

    const auto normalAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::ColorAttachment, TextureUsage::InputAttachment},
            TextureFormat::R32G32B32A32_SFLOAT,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Normal-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        { defaultTextureSampler }
    };

    const auto materialAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::ColorAttachment, TextureUsage::InputAttachment},
            TextureFormat::R32_UINT,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Material-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        { defaultTextureSampler }
    };

    const auto ambientAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::InputAttachment},
            TextureFormat::R8G8B8A8_SRGB,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Ambient-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        { defaultTextureSampler }
    };

    const auto diffuseAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::InputAttachment},
            TextureFormat::R8G8B8A8_SRGB,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Diffuse-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        { defaultTextureSampler }
    };

    const auto specularAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::InputAttachment},
            TextureFormat::R8G8B8A8_SRGB,
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Specular-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_COLOR_BIT, layerCount) },
        { defaultTextureSampler }
    };

    const auto depthAttachmentTexture = TextureDefinition {
        Texture::EmptyDepth(
            Render::TextureId::Invalid(),
            {TextureUsage::DepthStencilAttachment, TextureUsage::Sampled},
            renderSettings.resolution,
            layerCount,
            false,
            std::format("Depth-{}", tag)
        ),
        { TextureViewForLayerCount(TextureView::Aspect::ASPECT_DEPTH_BIT, layerCount) },
        { defaultTextureSampler }
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
        std::format("GPass-{}", tag)
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

std::optional<FrameBufferId> RenderTargets::CreateScreenFramebuffer(const std::string& tag) const
{
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    const auto defaultTextureSampler = TextureSampler(TextureSampler::DEFAULT, CLAMP_ADDRESS_MODE);

    auto nearestTextureSampler = TextureSampler(TextureSampler::NEAREST, CLAMP_ADDRESS_MODE);
    nearestTextureSampler.minFilter = SamplerFilterMode::Nearest;
    nearestTextureSampler.magFilter = SamplerFilterMode::Nearest;

    const auto colorAttachmentTexture = TextureDefinition {
        Texture::Empty(
            Render::TextureId::Invalid(),
            {TextureUsage::Sampled, TextureUsage::ColorAttachment, TextureUsage::TransferSource, TextureUsage::Storage},
            TextureFormat::R8G8B8A8_UNORM,
            renderSettings.resolution,
            1,
            false,
            std::format("ScreenColor-{}", tag)
        ),
        {
            TextureView::ViewAs2DArray(TextureView::DEFAULT, TextureView::Aspect::ASPECT_COLOR_BIT, TextureView::Layer(0, 1))
        },
        { defaultTextureSampler, nearestTextureSampler }
    };

    const auto depthAttachmentTexture = TextureDefinition {
        Texture::EmptyDepth(
            Render::TextureId::Invalid(),
            {TextureUsage::DepthStencilAttachment, TextureUsage::Sampled},
            renderSettings.resolution,
            1,
            false,
            std::format("ScreenDepth-{}", tag)
        ),
        { TextureView::ViewAs2D(TextureView::DEFAULT, TextureView::Aspect::ASPECT_DEPTH_BIT) },
        { defaultTextureSampler }
    };

    ////

    const auto screenFramebufferId = m_ids->frameBufferIds.GetId();

    const auto result = m_framebuffers->CreateFramebuffer(
        screenFramebufferId,
        m_vulkanObjs->GetScreenRenderPass(),
        {
            {colorAttachmentTexture, TextureView::DEFAULT},
            {depthAttachmentTexture, TextureView::DEFAULT}
        },
        m_vulkanObjs->GetRenderSettings().resolution,
        1,
        std::format("Screen-{}", tag)
    );

    if (!result)
    {
        m_ids->frameBufferIds.ReturnId(screenFramebufferId);

        m_logger->Log(Common::LogLevel::Error,
              "RenderTargets::CreateBlitFramebuffer: Failed to create screen framebuffer: {}", tag);

        return std::nullopt;
    }

    return screenFramebufferId;
}

std::optional<TextureId> RenderTargets::CreatePostProcessOutputTexture(const std::string& tag) const
{
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    uint32_t layerCount = 1;

    // If we're presenting to a headset, create two layers for each render target texture, to hold the output for each eye
    if (renderSettings.presentToHeadset)
    {
        layerCount = 2;
    }

    //
    // Create the texture
    //
    const auto attachmentTextureSampler = TextureSampler(TextureSampler::DEFAULT, CLAMP_ADDRESS_MODE);

    const auto textureId = m_ids->textureIds.GetId();

    if (!m_textures->CreateTextureEmpty(
        Texture::Empty(
            textureId,
            {TextureUsage::Storage, TextureUsage::TransferSource},
            TextureFormat::R8G8B8A8_UNORM, // Note that we're dropping down to 32bit color in compute output
            m_vulkanObjs->GetRenderSettings().resolution,
            layerCount,
            false,
            std::format("PostProcessOutput-{}", tag)
        ),
        { TextureView::ViewAs2DArray(
            TextureView::DEFAULT,
            TextureView::Aspect::ASPECT_COLOR_BIT,
            TextureView::Layer(0, layerCount)
        ) },
        { attachmentTextureSampler }
    ))
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreatePostProcessOutputTexture: Failed to create texture");
        return std::nullopt;
    }

    const auto loadedTexture = m_textures->GetTexture(textureId).value();

    return textureId;
}

void RenderTargets::DestroyRenderTarget(const RenderTargetId& renderTargetId, bool destroyImmediately)
{
    const auto it = m_renderTargets.find(renderTargetId);
    if (it == m_renderTargets.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Debug, "RenderTargets: Destroying render target: {}", renderTargetId.id);

    m_textures->DestroyTexture(it->second.postProcessOutputTexture, destroyImmediately);

    m_framebuffers->DestroyFramebuffer(it->second.screenFramebuffer, destroyImmediately);
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
