/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Textures.h"

#include "../VulkanObjs.h"
#include "../PostExecutionOp.h"
#include "../Metrics.h"

#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanPhysicalDevice.h"

#include "../VMA/IVMA.h"
#include "../Image/IImages.h"
#include "../Buffer/IBuffers.h"
#include "../Util/VulkanFuncs.h"
#include "../Util/Futures.h"

#include <Accela/Render/IVulkanCalls.h>

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include <format>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Accela::Render
{

Textures::Textures(Common::ILogger::Ptr logger,
                   Common::IMetrics::Ptr metrics,
                   VulkanObjsPtr vulkanObjs,
                   IImagesPtr images,
                   IBuffersPtr buffers,
                   PostExecutionOpsPtr postExecutionOps,
                   Ids::Ptr ids)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_images(std::move(images))
    , m_buffers(std::move(buffers))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_ids(std::move(ids))
{

}

bool Textures::Initialize()
{
    m_logger->Log(Common::LogLevel::Info, "Textures: Initializing");

    if (!CreateMissingTexture())
    {
        m_logger->Log(Common::LogLevel::Error, "Textures: Failed to create missing texture");
        return false;
    }

    SyncMetrics();

    return true;
}

void Textures::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Textures: Destroying");

    while (!m_textures.empty())
    {
        DestroyTexture(m_textures.cbegin()->first, true);
    }

    m_missingTextureId = TextureId{INVALID_ID};
    m_missingCubeTextureId = TextureId{INVALID_ID};

    SyncMetrics();
}

bool Textures::CreateTexture(const TextureDefinition& textureDefinition, std::promise<bool> resultPromise)
{
    const auto it = m_textures.find(textureDefinition.texture.id);
    if (it != m_textures.cend())
    {
        m_logger->Log(Common::LogLevel::Warning, "CreateTexture: Texture already exists: {}", textureDefinition.texture.id.id);
        return ErrorResult(resultPromise);
    }

    m_logger->Log(Common::LogLevel::Debug, "CreateTexture: Creating texture: {}", textureDefinition.texture.id.id);

    const auto imageDefinition = TextureDefToImageDef(textureDefinition);

    const auto imageIdExpect = m_images->CreateFilledImage(imageDefinition, textureDefinition.texture.data, std::move(resultPromise));
    if (!imageIdExpect)
    {
        m_logger->Log(Common::LogLevel::Warning, "CreateTexture: Failed to create image: {}", textureDefinition.texture.id.id);
        return false;
    }

    LoadedTexture loadedTexture{};
    loadedTexture.textureDefinition = textureDefinition;
    loadedTexture.imageId = *imageIdExpect;

    m_textures.insert(std::make_pair(textureDefinition.texture.id, loadedTexture));

    SyncMetrics();

    return true;
}

std::optional<LoadedTexture> Textures::GetTexture(TextureId textureId)
{
    const auto it = m_textures.find(textureId);
    if (it == m_textures.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

std::optional<std::pair<LoadedTexture, LoadedImage>> Textures::GetTextureAndImage(TextureId textureId)
{
    const auto loadedTexture = GetTexture(textureId);
    if (!loadedTexture)
    {
        return std::nullopt;
    }

    const auto loadedImage = m_images->GetImage(loadedTexture->imageId);
    if (!loadedImage)
    {
        return std::nullopt;
    }

    return std::make_pair(*loadedTexture, *loadedImage);
}

std::pair<LoadedTexture, LoadedImage> Textures::GetMissingTexture()
{
    return GetTextureAndImage(m_missingTextureId).value();
}

std::pair<LoadedTexture, LoadedImage> Textures::GetMissingCubeTexture()
{
    return GetTextureAndImage(m_missingCubeTextureId).value();
}

void Textures::DestroyTexture(TextureId textureId, bool destroyImmediately)
{
    const auto it = m_textures.find(textureId);
    if (it == m_textures.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
         "Textures: Asked to destroy texture which doesn't exist: {}", textureId.id);
        return;
    }

    const auto texture = it->second;

    m_images->DestroyImage(texture.imageId, destroyImmediately);

    m_textures.erase(texture.textureDefinition.texture.id);

    m_ids->textureIds.ReturnId(texture.textureDefinition.texture.id);

    SyncMetrics();
}

bool Textures::CreateMissingTexture()
{
    const unsigned int sizePx = 256;
    const unsigned int squareSizePx = 32;
    const std::array<std::byte, 4> squareOnColor{std::byte{255}, std::byte{0}, std::byte{255}, std::byte{255}};
    const std::array<std::byte, 4> squareOffColor{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}};

    std::vector<std::byte> missingTextureData(sizePx * sizePx * 4, std::byte{0});

    for (unsigned int y = 0; y < sizePx; ++y)
    {
        for (unsigned int x = 0; x < sizePx; ++x)
        {
            const unsigned int row = y / squareSizePx;
            const unsigned int col = x / squareSizePx;
            const bool on = (((row % 2) + (col % 2)) % 2) == 0;
            const auto color = on ? squareOnColor : squareOffColor;

            missingTextureData[(y * sizePx * 4) + (x * 4)] = color[0];
            missingTextureData[(y * sizePx * 4) + (x * 4) + 1] = color[1];
            missingTextureData[(y * sizePx * 4) + (x * 4) + 2] = color[2];
            missingTextureData[(y * sizePx * 4) + (x * 4) + 3] = color[3];
        }
    }

    //
    // Missing 2D Texture
    //
    const auto missingTextureImage = std::make_shared<Common::ImageData>(
        missingTextureData,
        1,
        sizePx,
        sizePx,
        Common::ImageData::PixelFormat::RGBA32
    );

    const auto missingTextureId = m_ids->textureIds.GetId();

    const auto missingTexture = Texture::FromImageData(
        missingTextureId,
        1,
        false,
        missingTextureImage,
        "Missing"
    );
    if (!missingTexture)
    {
        m_logger->Log(Common::LogLevel::Error, "Failed to create missing texture object");
        return false;
    }

    const auto missingTextureView = TextureView::ViewAs2D(TextureView::DEFAULT);

    //
    // Missing 2D Cube Texture
    //
    std::vector<std::byte> missingTextureCubeData;
    missingTextureCubeData.reserve(missingTextureData.size() * 6);

    for (unsigned int x = 0; x < 6; ++x)
    {
        std::ranges::copy(missingTextureData, std::back_inserter(missingTextureCubeData));
    }

    const auto missingTextureCubeImage = std::make_shared<Common::ImageData>(
        missingTextureCubeData,
        6,
        sizePx,
        sizePx,
        Common::ImageData::PixelFormat::RGBA32
    );

    const auto missingTextureCubeId = m_ids->textureIds.GetId();

    const auto missingTextureCube = Texture::FromImageData(
        missingTextureCubeId,
        6,
        true,
        missingTextureCubeImage,
        "MissingCube"
    );
    if (!missingTextureCube)
    {
        m_logger->Log(Common::LogLevel::Error, "Failed to create missing cube texture object");
        return false;
    }

    const auto missingTextureCubeView = TextureView::ViewAsCube(TextureView::DEFAULT);

    //
    // Create missing textures
    //
    const auto textureSampler = TextureSampler(TextureSampler::DEFAULT, WRAP_ADDRESS_MODE);

    // As this happens once during initialization, just create a fake promise/future for the data transfer,
    // we don't need to wait for it to finish
    std::promise<bool> createTexturePromise;
    std::future<bool> createTextureFuture = createTexturePromise.get_future();
    CreateTexture(
        TextureDefinition(*missingTexture, {missingTextureView}, {textureSampler}),
        std::move(createTexturePromise)
    );

    std::promise<bool> createTextureCubePromise;
    std::future<bool> createTextureCubeFuture = createTextureCubePromise.get_future();
    CreateTexture(
        TextureDefinition(*missingTextureCube, {missingTextureCubeView}, {textureSampler}),
        std::move(createTextureCubePromise)
    );

    m_missingTextureId = missingTextureId;
    m_missingCubeTextureId = missingTextureCubeId;

    return true;
}

void Textures::SyncMetrics() const
{
    m_metrics->SetCounterValue(Renderer_Textures_Count, m_textures.size());
}

ImageDefinition Textures::TextureDefToImageDef(const TextureDefinition& textureDefinition)
{
    VkFormat vkImageFormat{VK_FORMAT_R8G8B8A8_SRGB};

    switch (textureDefinition.texture.format)
    {
        case Format::RGBA32:
            vkImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        break;
    }

    // Textures are universally sampled and have their image data transfered to them
    VkImageUsageFlags vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // If the texture has mip levels, mark it as a transfer source for the mip-mapping blit transfers
    if (textureDefinition.texture.numMipLevels)
    {
        vkImageUsageFlags = vkImageUsageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    const auto image = Image{
        .tag = std::format("Texture-{}", textureDefinition.texture.tag),
        .vkImageType = VK_IMAGE_TYPE_2D,
        .vkFormat = vkImageFormat,
        .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
        .vkImageUsageFlags = vkImageUsageFlags,
        .size = textureDefinition.texture.pixelSize,
        .numLayers = textureDefinition.texture.numLayers,
        .numMipLevels = textureDefinition.texture.numMipLevels ? *textureDefinition.texture.numMipLevels : 1,
        .cubeCompatible = textureDefinition.texture.cubicTexture
    };

    std::vector<ImageView> imageViews;

    for (const auto& textureView : textureDefinition.textureViews)
    {
        VkImageViewType vkImageViewType{VK_IMAGE_VIEW_TYPE_2D};

        switch (textureView.viewType)
        {
            case TextureView::ViewType::VIEW_TYPE_2D:
                vkImageViewType = VK_IMAGE_VIEW_TYPE_2D;
                break;
            case TextureView::ViewType::VIEW_TYPE_CUBE:
                vkImageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
                break;
        }

        imageViews.push_back(ImageView{
            .name = textureView.name,
            .vkImageViewType = vkImageViewType,
            .vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseLayer = textureView.layer.baseLayer,
            .layerCount = textureView.layer.layerCount
        });
    }

    std::vector<ImageSampler> imageSamplers;

    for (const auto& textureSampler : textureDefinition.textureSamplers)
    {
        VkSamplerAddressMode uSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        switch (textureSampler.uvAddressMode.first)
        {
            case SamplerAddressMode::Wrap: uSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
            case SamplerAddressMode::Clamp: uSamplerMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
            case SamplerAddressMode::Mirror: uSamplerMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
        }

        VkSamplerAddressMode vSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        switch (textureSampler.uvAddressMode.second)
        {
            case SamplerAddressMode::Wrap: vSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
            case SamplerAddressMode::Clamp: vSamplerMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
            case SamplerAddressMode::Mirror: vSamplerMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
        }

        VkFilter vkMinFilter{};
        switch (textureSampler.minFilter)
        {
            case SamplerFilterMode::Nearest: vkMinFilter = VK_FILTER_NEAREST; break;
            case SamplerFilterMode::Linear: vkMinFilter = VK_FILTER_LINEAR; break;
        }

        VkFilter vkMagFilter{};
        switch (textureSampler.magFilter)
        {
            case SamplerFilterMode::Nearest: vkMagFilter = VK_FILTER_NEAREST; break;
            case SamplerFilterMode::Linear: vkMagFilter = VK_FILTER_LINEAR; break;
        }

        imageSamplers.push_back(ImageSampler{
            .name = textureSampler.name,
            .vkMagFilter = vkMagFilter,
            .vkMinFilter = vkMinFilter,
            .vkSamplerAddressModeU = uSamplerMode,
            .vkSamplerAddressModeV = vSamplerMode,
            .vkSamplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
        });
    }

    return ImageDefinition(image, imageViews, imageSamplers);
}

}
