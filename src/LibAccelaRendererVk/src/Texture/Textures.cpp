/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
#include "../Buffer/IBuffers.h"
#include "../Util/VulkanFuncs.h"

#include <Accela/Render/IVulkanCalls.h>

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include <format>
#include <cstddef>
#include <array>

namespace Accela::Render
{

Textures::Textures(Common::ILogger::Ptr logger,
                   Common::IMetrics::Ptr metrics,
                   VulkanObjsPtr vulkanObjs,
                   IBuffersPtr buffers,
                   PostExecutionOpsPtr postExecutionOps,
                   Ids::Ptr ids)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_buffers(std::move(buffers))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_ids(std::move(ids))
{

}

bool Textures::Initialize(VulkanCommandPoolPtr transferCommandPool,
                          VkQueue vkTransferQueue)
{
    m_logger->Log(Common::LogLevel::Info, "Textures: Initializing");

    m_transferCommandPool = transferCommandPool;
    m_vkTransferQueue = vkTransferQueue;

    if (!CreateMissingTexture())
    {
        m_logger->Log(Common::LogLevel::Error, "Textures: Failed to create missing texture");
        return false;
    }

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

    m_texturesLoading.clear();
    m_texturesToDestroy.clear();

    SyncMetrics();
}

bool Textures::CreateTextureEmpty(const Texture& texture, const std::vector<TextureView>& textureViews, const TextureSampler& textureSampler)
{
    if (texture.data.has_value())
    {
        m_logger->Log(Common::LogLevel::Warning, "CreateTextureEmpty: Texture has data provided, will be ignored: {}", texture.id.id);
    }

    const auto it = m_textures.find(texture.id);
    if (it != m_textures.cend())
    {
        m_logger->Log(Common::LogLevel::Warning, "CreateTextureEmpty: Texture already exists: {}", texture.id.id);
        return false;
    }

    m_logger->Log(Common::LogLevel::Debug, "CreateTextureEmpty: Creating empty texture objects: {}", texture.id.id);

    const auto loadedTexture = CreateTextureObjects(texture, textureViews, textureSampler, 1, false);
    if (!loadedTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateTextureEmpty: Failed to create texture objects for texture: {}", texture.id.id);
        return false;
    }

    m_textures.insert(std::make_pair(texture.id, loadedTexture.value()));
    SyncMetrics();

    return true;
}

void Textures::CreateTextureFilled(const Texture& texture,
                                   const std::vector<TextureView>& textureViews,
                                   const TextureSampler& textureSampler,
                                   bool generateMipMapsRequested,
                                   std::promise<bool> resultPromise)
{
    if (!texture.data.has_value())
    {
        m_logger->Log(Common::LogLevel::Error, "CreateTextureFilled: Texture has no data provided: {}", texture.id.id);
        resultPromise.set_value(false);
        return;
    }

    const auto it = m_textures.find(texture.id);
    if (it != m_textures.cend())
    {
        m_logger->Log(Common::LogLevel::Warning, "CreateTextureFilled: Texture already exists: {}", texture.id.id);
        resultPromise.set_value(false);
        return;
    }

    //
    // Mipmap setup
    //
    bool generateMipMaps = false;
    uint32_t mipLevels = 1;

    if (generateMipMapsRequested)
    {
        const auto vkFormat = VulkanFuncs::ImageDataFormatToVkFormat((*texture.data)->GetPixelFormat());
        if (vkFormat)
        {
            const bool deviceSupportsMipMaps = DoesDeviceSupportMipMapGeneration(*vkFormat);
            const bool imageSupportsMipMaps = texture.numLayers == 1;

            generateMipMaps = deviceSupportsMipMaps && imageSupportsMipMaps;

            if (!generateMipMaps)
            {
                m_logger->Log(Common::LogLevel::Warning,
                  "CreateTextureFilled: Asked to generate mipmaps but device or image doesn't support it, ignoring");
            }
        }
    }

    if (generateMipMaps)
    {
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texture.pixelSize.w, texture.pixelSize.h)))) + 1;
    }

    //
    // Create the texture's objects
    //
    m_logger->Log(Common::LogLevel::Debug, "CreateTextureFilled: Creating texture objects: {}", texture.id.id);

    const auto loadedTexture = CreateTextureObjects(texture, textureViews, textureSampler, mipLevels, generateMipMaps);
    if (!loadedTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateTextureFilled: Failed to create texture objects for texture: {}", texture.id.id);
        resultPromise.set_value(false);
        return;
    }

    m_textures.insert(std::make_pair(texture.id, loadedTexture.value()));
    SyncMetrics();

    //
    // Asynchronously fill the texture image with the provided data, and generate mipmaps as needed
    //
    m_logger->Log(Common::LogLevel::Debug, "CreateTextureFilled: Starting data transfer for texture: {}", texture.id.id);

    FillImageWithData(*loadedTexture, *(texture.data), mipLevels, generateMipMaps, std::move(resultPromise));
}

std::expected<LoadedTexture, bool> Textures::CreateTextureObjects(const Texture& texture,
                                                                  const std::vector<TextureView>& textureViews,
                                                                  const TextureSampler& textureSampler,
                                                                  const uint32_t& mipLevels,
                                                                  bool generatingMipMaps)
{
    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    LoadedTexture loadedTexture{};
    loadedTexture.textureId = texture.id;
    loadedTexture.pixelSize = texture.pixelSize;
    loadedTexture.mipLevels = mipLevels;
    loadedTexture.numLayers = texture.numLayers;

    //
    // Setup
    //
    const auto vkFormat = GetTextureImageFormat(texture);
    if (!vkFormat)
    {
        m_logger->Log(Common::LogLevel::Error, "CreateTextureObjects: Unable to determine texture image format");
        return std::unexpected(false);
    }

    loadedTexture.vkFormat = *vkFormat;

    //
    // Create a VkImage, VkImageView, VkSampler
    //
    if (!CreateTextureImage(loadedTexture, texture, generatingMipMaps))
    {
        m_logger->Log(Common::LogLevel::Error, "CreateTextureObjects: Failed to create texture image");
        return std::unexpected(false);
    }

    for (const auto& textureView : textureViews)
    {
        if (!CreateTextureImageView(loadedTexture, textureView))
        {
            m_logger->Log(Common::LogLevel::Error, "CreateTextureObjects: Failed to create texture image view");
            return std::unexpected(false);
        }
    }

    if (!CreateTextureImageSampler(loadedTexture, textureSampler))
    {
        m_logger->Log(Common::LogLevel::Error, "CreateTextureObjects: Failed to create texture image sampler");
        return std::unexpected(false);
    }

    //
    // Now that all operations are successful, attach debug names to the created objects
    //
    const std::string textureTag = std::format("Texture-{}", texture.tag);

    SetDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_IMAGE, (uint64_t)loadedTexture.allocation.vkImage, "Image-" + textureTag);
    for (const auto& vkImageViewIt : loadedTexture.vkImageViews)
    {
        SetDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkImageViewIt.second, "ImageView-" + textureTag);
    }
    SetDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_SAMPLER, (uint64_t)loadedTexture.vkSampler, "Sampler-" + textureTag);

    return loadedTexture;
}

bool Textures::CreateTextureImage(LoadedTexture& loadedTexture, const Texture& texture, bool generatingMipMaps) const
{
    //
    // Extent
    //
    VkExtent3D vkExtent{};
    vkExtent.width = texture.pixelSize.w;
    vkExtent.height = texture.pixelSize.h;
    vkExtent.depth = 1;

    //
    // Usage
    //
    VkImageUsageFlags vkImageUsageFlags{0};

    switch (texture.usage)
    {
        case TextureUsage::ImageMaterial:
        case TextureUsage::ImageCubeMaterial:
            vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
        break;
        case TextureUsage::ColorAttachment:
            vkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        break;
        case TextureUsage::InputAttachment_RGBA16_SFLOAT:
        case TextureUsage::InputAttachment_R32_UINT:
            vkImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        break;
        case TextureUsage::DepthAttachment:
        case TextureUsage::DepthCubeAttachment:
            vkImageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT;
        break;
    }

    vkImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // If we're generating mipmaps, mark the image as a transfer source as
    // we blit from the base mip level to the other mip levels
    if (generatingMipMaps)
    {
        vkImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    //
    // Create a VkImage
    //
    VkImageCreateFlags vkImageCreateFlags{0};

    if (texture.usage == TextureUsage::ImageCubeMaterial ||
        texture.usage == TextureUsage::DepthCubeAttachment)
    {
        vkImageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = loadedTexture.vkFormat;
    info.extent = vkExtent;
    info.mipLevels = loadedTexture.mipLevels;
    info.arrayLayers = texture.numLayers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = vkImageUsageFlags;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.flags = vkImageCreateFlags;

    VmaAllocationCreateInfo vmaAllocCreateInfo = {};
    vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocCreateInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImage vkImage;
    VmaAllocation vmaAllocation;

    auto result = m_vulkanObjs->GetVMA()->CreateImage(&info, &vmaAllocCreateInfo, &vkImage, &vmaAllocation, nullptr);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateTextureImage: vmaCreateImage failure, result code: {}", (uint32_t)result);
        return false;
    }

    loadedTexture.allocation = ImageAllocation(vkImage, vmaAllocation);

    return true;
}

bool Textures::CreateTextureImageView(LoadedTexture& loadedTexture,
                                      const TextureView& textureView) const
{
    if (loadedTexture.vkImageViews.contains(textureView.name))
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateTextureImageView: Texture already has a texture view with the name: {}", textureView.name);
        return false;
    }

    VkImageViewType vkImageViewType{};
    switch (textureView.viewType)
    {
        case TextureView::ViewType::VIEW_TYPE_1D: vkImageViewType = VK_IMAGE_VIEW_TYPE_1D; break;
        case TextureView::ViewType::VIEW_TYPE_2D: vkImageViewType = VK_IMAGE_VIEW_TYPE_2D; break;
        case TextureView::ViewType::VIEW_TYPE_3D: vkImageViewType = VK_IMAGE_VIEW_TYPE_3D; break;
        case TextureView::ViewType::VIEW_TYPE_CUBE: vkImageViewType = VK_IMAGE_VIEW_TYPE_CUBE; break;
        case TextureView::ViewType::VIEW_TYPE_1D_ARRAY: vkImageViewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY; break;
        case TextureView::ViewType::VIEW_TYPE_2D_ARRAY: vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
        case TextureView::ViewType::VIEW_TYPE_CUBE_ARRAY: vkImageViewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
    }

    VkImageAspectFlags vkImageAspectFlags{0};
    for (const auto& aspect : textureView.aspects)
    {
        switch (aspect)
        {
            case TextureView::Aspect::ASPECT_COLOR_BIT: vkImageAspectFlags = vkImageAspectFlags | VK_IMAGE_ASPECT_COLOR_BIT; break;
            case TextureView::Aspect::ASPECT_DEPTH_BIT: vkImageAspectFlags = vkImageAspectFlags | VK_IMAGE_ASPECT_DEPTH_BIT; break;
            case TextureView::Aspect::ASPECT_STENCIL_BIT: vkImageAspectFlags = vkImageAspectFlags | VK_IMAGE_ASPECT_STENCIL_BIT; break;
        }
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = loadedTexture.allocation.vkImage;
    viewInfo.viewType = vkImageViewType;
    viewInfo.format = loadedTexture.vkFormat;
    viewInfo.subresourceRange.aspectMask = vkImageAspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = loadedTexture.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = textureView.layer.baseLayer;
    viewInfo.subresourceRange.layerCount = textureView.layer.layerCount;

    VkImageView vkImageView{VK_NULL_HANDLE};

    auto result = m_vulkanObjs->GetCalls()->vkCreateImageView(m_vulkanObjs->GetDevice()->GetVkDevice(), &viewInfo, nullptr, &vkImageView);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateTextureImageView: vkCreateImageView call failed, result code: {}", (uint32_t)result);

        m_vulkanObjs->GetVMA()->DestroyImage(loadedTexture.allocation.vkImage, loadedTexture.allocation.vmaAllocation);

        return false;
    }

    loadedTexture.vkImageViews.insert({textureView.name, vkImageView});

    return true;
}

bool Textures::CreateTextureImageSampler(LoadedTexture& loadedTexture, const TextureSampler& textureSampler) const
{
    VkSamplerAddressMode uSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    switch (textureSampler.uvAddressMode.first)
    {
        case SamplerAddressMode::Wrap: uSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
        case SamplerAddressMode::Clamp: uSamplerMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
    }

    VkSamplerAddressMode vSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    switch (textureSampler.uvAddressMode.second)
    {
        case SamplerAddressMode::Wrap: vSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
        case SamplerAddressMode::Clamp: vSamplerMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = uSamplerMode;
    samplerInfo.addressModeV = vSamplerMode;
    samplerInfo.addressModeW = samplerInfo.addressModeU; // Noteworthy
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // Configure mipmap sampling if we have mip levels
    if (loadedTexture.mipLevels > 1)
    {
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(loadedTexture.mipLevels);
        samplerInfo.mipLodBias = 0.0f;
    }

    VkSampler vkSampler{VK_NULL_HANDLE};

    auto result = m_vulkanObjs->GetCalls()->vkCreateSampler(m_vulkanObjs->GetDevice()->GetVkDevice(), &samplerInfo, nullptr, &vkSampler);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateTextureImageSampler: vkCreateSampler call failed, result code: {}", (uint32_t)result);

        for (const auto& vkImageViewIt : loadedTexture.vkImageViews)
        {
            m_vulkanObjs->GetCalls()->vkDestroyImageView(m_vulkanObjs->GetDevice()->GetVkDevice(), vkImageViewIt.second, nullptr);
        }
        m_vulkanObjs->GetVMA()->DestroyImage(loadedTexture.allocation.vkImage, loadedTexture.allocation.vmaAllocation);

        return false;
    }

    loadedTexture.vkSampler = vkSampler;

    return true;
}

void Textures::DestroyTextureObjects(const LoadedTexture& texture) const
{
    m_logger->Log(Common::LogLevel::Debug, "Textures: Destroying texture objects: {}", texture.textureId.id);

    // Remove debug names
    RemoveDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_IMAGE, (uint64_t)texture.allocation.vkImage);
    for (const auto& vkImageViewIt : texture.vkImageViews)
    {
        RemoveDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkImageViewIt.second);
    }
    RemoveDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_SAMPLER, (uint64_t)texture.vkSampler);

    // Destroy objects
    m_vulkanObjs->GetCalls()->vkDestroySampler(m_vulkanObjs->GetDevice()->GetVkDevice(), texture.vkSampler, nullptr);
    for (const auto& vkImageViewIt : texture.vkImageViews)
    {
        m_vulkanObjs->GetCalls()->vkDestroyImageView(m_vulkanObjs->GetDevice()->GetVkDevice(), vkImageViewIt.second, nullptr);
    }
    m_vulkanObjs->GetVMA()->DestroyImage(texture.allocation.vkImage, texture.allocation.vmaAllocation);

    // Return the id to the pool now that it's fully no longer in use
    m_ids->textureIds.ReturnId(texture.textureId);
}

std::expected<VkFormat, bool> Textures::GetTextureImageFormat(const Texture& texture)
{
    std::optional<VkFormat> vkFormat;

    if (texture.data.has_value())
    {
        vkFormat = VulkanFuncs::ImageDataFormatToVkFormat((*texture.data)->GetPixelFormat());
    }

    if (!vkFormat)
    {
        switch (texture.usage)
        {
            case TextureUsage::ColorAttachment:
                vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            break;
            case TextureUsage::DepthAttachment:
            case TextureUsage::DepthCubeAttachment:
                vkFormat = m_vulkanObjs->GetPhysicalDevice()->GetDepthBufferFormat();
            break;
            case TextureUsage::InputAttachment_RGBA16_SFLOAT:
                vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
            break;
            case TextureUsage::InputAttachment_R32_UINT:
                vkFormat = VK_FORMAT_R32_UINT;
            break;
            case TextureUsage::ImageMaterial:
            case TextureUsage::ImageCubeMaterial:
                assert(false);
            break;
        }
    }

    if (!vkFormat)
    {
        m_logger->Log(Common::LogLevel::Error, "GetTextureImageFormat: Unsupported texture format");
        return std::unexpected(false);
    }

    return *vkFormat;
}

bool Textures::DoesDeviceSupportMipMapGeneration(const VkFormat& vkFormat) const
{
    auto vulkanFuncs = VulkanFuncs(m_logger, m_vulkanObjs);

    const VkFormatProperties vkFormatProperties = vulkanFuncs.GetVkFormatProperties(vkFormat);

    return vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
}

// TODO: If this method can ever be called more than once for a texture, it needs to be able to handle multiple parallel transfers
void Textures::FillImageWithData(const LoadedTexture& loadedTexture,
                                 const Common::ImageData::Ptr& imageData,
                                 const uint32_t& mipLevels,
                                 bool generateMipMaps,
                                 std::promise<bool> resultPromise)
{
    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    const auto loadingTexture = std::make_shared<LoadingTexture>(std::move(resultPromise));

    // Mark the texture as loading
    m_texturesLoading.insert({loadedTexture.textureId, loadingTexture});

    vulkanFuncs.QueueSubmit(
        std::format("FillImageWithData-{}", loadedTexture.textureId.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [&](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)  {

            // After the data transfer the image should be ready to be read by a shader
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // ... Unless we need to generate mipmaps, in which case it should be instead
            // be put into transfer dest optimal for receiving mip data
            if (generateMipMaps)
            {
                finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }

            //
            // Transfer from the provided data to the image's base mip level
            //
            const auto transferResult = vulkanFuncs.TransferImageData(
                m_buffers,
                m_postExecutionOps,
                commandBuffer->GetVkCommandBuffer(),
                vkFence,
                imageData,
                loadedTexture.allocation.vkImage,
                mipLevels,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                finalLayout
            );
            if (!transferResult)
            {
                m_logger->Log(Common::LogLevel::Error, "TransferImageToTexture: Failed to transfer data to GPU image");
            }

            //
            // If requested, generate mip maps for the image's other mip levels
            //
            if (generateMipMaps)
            {
                vulkanFuncs.GenerateMipMaps(
                    commandBuffer->GetVkCommandBuffer(),
                    USize(imageData->GetPixelWidth(), imageData->GetPixelHeight()),
                    loadedTexture.allocation.vkImage,
                    mipLevels,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );
            }

            // When the transfer's work/fence has finished, handle post load tasks
            m_postExecutionOps->EnqueueFrameless(vkFence, [=,this](){ OnTextureLoadFinished(loadedTexture); });
        }
    );
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

LoadedTexture Textures::GetMissingTexture()
{
    return GetTexture(m_missingTextureId).value();
}

LoadedTexture Textures::GetMissingCubeTexture()
{
    return GetTexture(m_missingCubeTextureId).value();
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

    // Whether destroying the texture's objects immediately or not below, erase our knowledge
    // of the texture; no future render work is allowed to use it
    m_textures.erase(it);

    ////

    // If a texture's data transfer is still happening, need to wait until the transfer has finished before
    // destroying the texture's Vulkan objects. Mark the texture as to be deleted and bail out.
    if (m_texturesLoading.contains(textureId) && !destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Textures: Postponing destroy of texture: {}", textureId.id);
        m_texturesToDestroy.insert(textureId);
    }
    else if (destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Textures: Destroying texture immediately: {}", textureId.id);
        DestroyTextureObjects(texture);
    }
    else
    {
        m_logger->Log(Common::LogLevel::Debug, "Textures: Enqueueing texture destroy: {}", textureId.id);
        m_postExecutionOps->Enqueue_Current([=,this]() { DestroyTextureObjects(texture); });
    }

    SyncMetrics();
}

void Textures::OnTextureLoadFinished(const LoadedTexture& loadedTexture)
{
    m_logger->Log(Common::LogLevel::Debug, "Textures: Texture load finished for texture: {}", loadedTexture.textureId.id);

    const auto it = m_texturesLoading.find(loadedTexture.textureId);
    if (it == m_texturesLoading.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "Textures: Texture load finished for texture while has no loading record: {}", loadedTexture.textureId.id);
        return;
    }

    // Mark the texture as finished loading
    it->second->resultPromise.set_value(true);

    // The texture is no longer loading
    m_texturesLoading.erase(it);

    // If while the load was in progress we were told to destroy the texture, go ahead and do it now
    if (m_texturesToDestroy.contains(loadedTexture.textureId))
    {
        m_logger->Log(Common::LogLevel::Debug, "Textures: Loaded texture should be destroyed: {}", loadedTexture.textureId.id);

        m_texturesToDestroy.erase(loadedTexture.textureId);

        // Enqueuing the objects destruction as this method's callback is frameless, and we want to wait for all
        // in-progress frames to finish before destroying the texture's objects
        m_postExecutionOps->Enqueue_Current([=,this]() { DestroyTextureObjects(loadedTexture); });
    }

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
    const auto missingTexture = Texture::FromImageData(missingTextureId, TextureUsage::ImageMaterial, 1, missingTextureImage, "Missing");
    const auto missingTextureView = TextureView::ViewAs2D(TextureView::DEFAULT, TextureView::Aspect::ASPECT_COLOR_BIT);

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
    const auto missingTextureCube = Texture::FromImageData(missingTextureCubeId, TextureUsage::ImageCubeMaterial, 6, missingTextureCubeImage, "MissingCube");
    const auto missingTextureCubeView = TextureView::ViewAsCube(TextureView::DEFAULT, TextureView::Aspect::ASPECT_COLOR_BIT);

    const auto textureSampler = TextureSampler(WRAP_ADDRESS_MODE);

    // As this happens once during initialization, just create a fake promise/future for the data transfer,
    // we don't need to wait for it to finish
    std::promise<bool> createTexturePromise;
    std::future<bool> createTextureFuture = createTexturePromise.get_future();
    CreateTextureFilled(missingTexture, {missingTextureView}, textureSampler, false, std::move(createTexturePromise));

    std::promise<bool> createTextureCubePromise;
    std::future<bool> createTextureCubeFuture = createTextureCubePromise.get_future();
    CreateTextureFilled(missingTextureCube, {missingTextureCubeView}, textureSampler, false, std::move(createTextureCubePromise));

    m_missingTextureId = missingTextureId;
    m_missingCubeTextureId = missingTextureCubeId;

    return true;
}

void Textures::SyncMetrics()
{
    m_metrics->SetCounterValue(Renderer_Textures_Count, m_textures.size());
    m_metrics->SetCounterValue(Renderer_Textures_Loading_Count, m_texturesLoading.size());
    m_metrics->SetCounterValue(Renderer_Textures_ToDestroy_Count, m_texturesToDestroy.size());
}

}
