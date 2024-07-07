/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderTargets.h"

#include "../VulkanObjs.h"
#include "../InternalCommon.h"

#include "../Framebuffer/IFramebuffers.h"
#include "../Image/IImages.h"
#include "../Util/VulkanFuncs.h"
#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanPhysicalDevice.h"

#include <format>

namespace Accela::Render
{

RenderTargets::RenderTargets(Common::ILogger::Ptr logger,
                             VulkanObjsPtr vulkanObjs,
                             PostExecutionOpsPtr postExecutionOps,
                             IFramebuffersPtr framebuffers,
                             IImagesPtr images,
                             Ids::Ptr ids)
     : m_logger(std::move(logger))
     , m_vulkanObjs(std::move(vulkanObjs))
     , m_postExecutionOps(std::move(postExecutionOps))
     , m_framebuffers(std::move(framebuffers))
     , m_images(std::move(images))
     , m_ids(std::move(ids))
{

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

    const auto postProcessOutputImage = CreatePostProcessOutputImage(tag);
    if (!postProcessOutputImage)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreateRenderTarget: Failed to create post-process output image: {}",tag);
        return false;
    }

    m_renderTargets.insert({
        renderTargetId,
        RenderTarget(*gPassFramebufferId, *screenFramebufferId, *postProcessOutputImage, tag)
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

    const auto defaultImageSampler = ImageSampler(
        ImageSampler::DEFAULT,
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_MIPMAP_MODE_LINEAR
    );

    const auto nearestImageSampler = ImageSampler(
        ImageSampler::NEAREST,
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_MIPMAP_MODE_LINEAR
    );

    //
    // Color Attachment Image
    //
    const auto colorAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("Color-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                | VK_IMAGE_USAGE_STORAGE_BIT,
            .size = renderSettings.resolution,
            .numLayers = layerCount,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseLayer = 0,
                .layerCount = layerCount
            }
        },
        { defaultImageSampler, nearestImageSampler }
    };

    //
    // Position Attachment Image
    //
    const auto positionAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("Position-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT,
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            .size = renderSettings.resolution,
            .numLayers = layerCount,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseLayer = 0,
                .layerCount = layerCount
            }
        },
        { nearestImageSampler }
    };

    //
    // Normal Attachment Image
    //
    auto normalAttachmentImage = positionAttachmentImage;
    normalAttachmentImage.image.tag = std::format("Normal-{}", tag);

    //
    // Object Detail Attachment Image
    //

    // R = Object ID, G = Material ID
    auto objectDetailImageSampler = nearestImageSampler;
    objectDetailImageSampler.vkSamplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; // R32G32 doesn't support linear

    const auto objectDetailAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("ObjectDetail-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = GetObjectDetailVkFormat(),
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .size = renderSettings.resolution,
            .numLayers = layerCount,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseLayer = 0,
                .layerCount = layerCount
            }
        },
        { objectDetailImageSampler }
    };

    //
    // Ambient Attachment Image
    //
    const auto ambientAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("Ambient-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = VK_FORMAT_R8G8B8A8_SRGB,
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            .size = renderSettings.resolution,
            .numLayers = layerCount,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseLayer = 0,
                .layerCount = layerCount
            }
        },
        { nearestImageSampler }
    };

    //
    // Diffuse Attachment Image
    //
    auto diffuseAttachmentImage = ambientAttachmentImage;
    diffuseAttachmentImage.image.tag = std::format("Diffuse-{}", tag);

    //
    // Specular Attachment Image
    //
    auto specularAttachmentImage = ambientAttachmentImage;
    specularAttachmentImage.image.tag = std::format("Specular-{}", tag);

    //
    // Depth Attachment Image
    //
    const auto depthAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("Depth-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = m_vulkanObjs->GetPhysicalDevice()->GetDepthBufferFormat(),
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .size = renderSettings.resolution,
            .numLayers = layerCount,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseLayer = 0,
                .layerCount = layerCount
            }
        },
        { nearestImageSampler }
    };

    //
    // Create the GPass Framebuffer
    //

    const auto gPassFramebufferId = m_ids->frameBufferIds.GetId();

    const bool result = m_framebuffers->CreateFramebuffer(
        gPassFramebufferId,
        m_vulkanObjs->GetGPassRenderPass(),
        {
            {colorAttachmentImage, ImageView::DEFAULT},
            {positionAttachmentImage, ImageView::DEFAULT},
            {normalAttachmentImage, ImageView::DEFAULT},
            {objectDetailAttachmentImage, ImageView::DEFAULT},
            {ambientAttachmentImage, ImageView::DEFAULT},
            {diffuseAttachmentImage, ImageView::DEFAULT},
            {specularAttachmentImage, ImageView::DEFAULT},
            {depthAttachmentImage, ImageView::DEFAULT}
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

    const auto defaultImageSampler = ImageSampler(
        ImageSampler::DEFAULT,
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_MIPMAP_MODE_LINEAR
    );

    const auto nearestImageSampler = ImageSampler(
        ImageSampler::NEAREST,
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_MIPMAP_MODE_LINEAR
    );

    const auto colorAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("ScreenColor-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = VK_FORMAT_R8G8B8A8_UNORM,
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                 | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                 | VK_IMAGE_USAGE_STORAGE_BIT,
            .size = renderSettings.resolution,
            .numLayers = 1,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseLayer = 0,
                .layerCount = 1
            }
        },
        { defaultImageSampler, nearestImageSampler }
    };

    const auto depthAttachmentImage = ImageDefinition {
        Image{
            .tag = std::format("ScreenDepth-{}", tag),
            .vkImageType = VK_IMAGE_TYPE_2D,
            .vkFormat = m_vulkanObjs->GetPhysicalDevice()->GetDepthBufferFormat(),
            .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
            .vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .size = renderSettings.resolution,
            .numLayers = 1,
            .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        },
        {
            ImageView{
                .name = ImageView::DEFAULT,
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseLayer = 0,
                .layerCount = 1
            }
        },
        { defaultImageSampler }
    };

    ////

    const auto screenFramebufferId = m_ids->frameBufferIds.GetId();

    const auto result = m_framebuffers->CreateFramebuffer(
        screenFramebufferId,
        m_vulkanObjs->GetScreenRenderPass(),
        {
            {colorAttachmentImage, ImageView::DEFAULT},
            {depthAttachmentImage, ImageView::DEFAULT}
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

std::optional<ImageId> RenderTargets::CreatePostProcessOutputImage(const std::string& tag) const
{
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    uint32_t layerCount = 1;

    // If we're presenting to a headset, create two layers for each render target texture, to hold the output for each eye
    if (renderSettings.presentToHeadset)
    {
        layerCount = 2;
    }

    //
    // Create the image
    //
    const Image image{
        .tag = std::format("PostProcessOutput-{}", tag),
        .vkImageType = VK_IMAGE_TYPE_2D,
        .vkFormat = VK_FORMAT_R8G8B8A8_UNORM,
        .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
        .vkImageUsageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .size = m_vulkanObjs->GetRenderSettings().resolution,
        .numLayers = layerCount,
        .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    };

    const ImageView imageView{
        .name = ImageView::DEFAULT,
        .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseLayer = 0,
        .layerCount = layerCount
    };

    const ImageSampler imageSampler{
        .name = ImageSampler::DEFAULT,
        .vkMagFilter = VK_FILTER_LINEAR,
        .vkMinFilter = VK_FILTER_LINEAR,
        .vkSamplerAddressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .vkSamplerAddressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .vkSamplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    const auto imageExpect = m_images->CreateEmptyImage({image, {imageView}, {imageSampler}});
    if (!imageExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderTargets::CreatePostProcessOutputImage: Failed to create image");
        return std::nullopt;
    }

    return *imageExpect;
}

void RenderTargets::DestroyRenderTarget(const RenderTargetId& renderTargetId, bool destroyImmediately)
{
    const auto it = m_renderTargets.find(renderTargetId);
    if (it == m_renderTargets.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Debug, "RenderTargets: Destroying render target: {}", renderTargetId.id);

    m_images->DestroyImage(it->second.postProcessOutputImage, destroyImmediately);

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

VkFormat RenderTargets::GetObjectDetailVkFormat() const
{
    return VK_FORMAT_R32G32_UINT;
}

std::size_t RenderTargets::GetObjectDetailPerPixelByteSize() const
{
    // VK_FORMAT_R32G32_UINT
    return 8;
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
