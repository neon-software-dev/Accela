/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Images.h"

#include "../VulkanObjs.h"
#include "../PostExecutionOps.h"
#include "../Metrics.h"

#include "../Buffer/IBuffers.h"
#include "../VMA/IVMA.h"
#include "../Util/VulkanFuncs.h"
#include "../Util/Futures.h"
#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanPhysicalDevice.h"
#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanCommandBuffer.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

Images::Images(Common::ILogger::Ptr logger,
               Common::IMetrics::Ptr metrics,
               VulkanObjsPtr vulkanObjs,
               IBuffersPtr buffers,
               PostExecutionOpsPtr postExecutionOps)
   : m_logger(std::move(logger))
   , m_metrics(std::move(metrics))
   , m_vulkanObjs(std::move(vulkanObjs))
   , m_buffers(std::move(buffers))
   , m_postExecutionOps(std::move(postExecutionOps))
{

}

bool Images::Initialize(VulkanCommandPoolPtr transferCommandPool, VkQueue vkTransferQueue)
{
    m_logger->Log(Common::LogLevel::Info, "Images: Initializing");

    m_transferCommandPool = transferCommandPool;
    m_vkTransferQueue = vkTransferQueue;

    SyncMetrics();

    return true;
}

void Images::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Images: Destroying");

    while (!m_images.empty())
    {
        DestroyImage(m_images.cbegin()->first, true);
    }

    m_imagesLoading.clear();
    m_imagesToDestroy.clear();

    m_transferCommandPool = nullptr;
    m_vkTransferQueue = VK_NULL_HANDLE;

    SyncMetrics();
}

std::expected<ImageId, bool> Images::CreateEmptyImage(const ImageDefinition& imageDefinition)
{
    m_logger->Log(Common::LogLevel::Debug, "Images: Creating empty image: {}", imageDefinition.image.tag);

    //
    // Create image objects
    //
    const auto loadedImageExpect = CreateImageObjects(imageDefinition);
    if (!loadedImageExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateEmptyImage: Failed to create image objects for: {}", imageDefinition.image.tag);
        return std::unexpected(false);
    }

    auto loadedImage = *loadedImageExpect;

    //
    // Record result
    //
    loadedImage.id = m_imageIds.GetId();

    m_images.insert({loadedImage.id, loadedImage});

    SyncMetrics();

    return loadedImage.id;
}

std::expected<ImageId, bool> Images::CreateFilledImage(const ImageDefinition& imageDefinition,
                                                       const Common::ImageData::Ptr& data,
                                                       std::promise<bool> resultPromise)
{
    m_logger->Log(Common::LogLevel::Debug, "Images: Creating filled image: {}", imageDefinition.image.tag);

    //
    // Create image objects
    //
    const auto loadedImageExpect = CreateImageObjects(imageDefinition);
    if (!loadedImageExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateFilledImage: Failed to create image objects for: {}", imageDefinition.image.tag);
        return std::unexpected(false);
    }

    auto loadedImage = *loadedImageExpect;

    //
    // Record result
    //
    loadedImage.id = m_imageIds.GetId();

    m_images.insert({loadedImage.id, loadedImage});

    SyncMetrics();

    //
    // Start an asynchronous data transfer to the image
    //
    if (!TransferImageData(loadedImage, data, true, std::move(resultPromise)))
    {
        m_logger->Log(Common::LogLevel::Error,
              "Images::CreateFilledImage: Failed to transfer initial image data for: {}", imageDefinition.image.tag);
        // Note that we don't count data transfer failure as an error that would return unexpected
    }

    return loadedImage.id;
}

std::expected<LoadedImage, bool> Images::CreateImageObjects(const ImageDefinition& imageDefinition)
{
    //
    // Create the VkImage/allocation
    //
    auto loadedImageExpect = CreateVkImage(imageDefinition.image);
    if (!loadedImageExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateImageObjects: Failed to create VkImage for: {}", imageDefinition.image.tag);
        return std::unexpected(false);
    }

    auto loadedImage = *loadedImageExpect;

    //
    // Create VkImageViews
    //
    for (const auto& imageView : imageDefinition.imageViews)
    {
        if (!CreateVkImageView(imageView, loadedImage))
        {
            m_logger->Log(Common::LogLevel::Error,
              "Images::CreateImageObjects: Failed to create VkImageView: {} for: {}", imageView.name, imageDefinition.image.tag);
            DestroyImageObjects(loadedImage);
            return std::unexpected(false);
        }
    }

    //
    // Create VkSamplers
    //
    for (const auto& imageSampler : imageDefinition.imageSamplers)
    {
        if (!CreateVkImageSampler(imageSampler, loadedImage))
        {
            m_logger->Log(Common::LogLevel::Error,
              "Images::CreateImageObjects: Failed to create VkImageSampler: {} for: {}", imageSampler.name, imageDefinition.image.tag);
            DestroyImageObjects(loadedImage);
            return std::unexpected(false);
        }
    }

    return loadedImage;
}

std::expected<LoadedImage, bool> Images::CreateVkImage(const Image& image) const
{
    VkExtent3D vkExtent{};
    vkExtent.width = image.size.w;
    vkExtent.height = image.size.h;
    vkExtent.depth = 1;

    VkImageCreateFlags vkImageCreateFlags{0};

    if (image.cubeCompatible)
    {
        if (image.numLayers != 6)
        {
            m_logger->Log(Common::LogLevel::Error,
              "Images::CreateVkImage: Specified as cube compatible, but doesn't have six layers: {}", image.tag);
            return std::unexpected(false);
        }

        vkImageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.imageType = image.vkImageType;
    info.format = image.vkFormat;
    info.extent = vkExtent;
    info.mipLevels = image.numMipLevels;
    info.arrayLayers = image.numLayers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = image.vkImageTiling;
    info.usage = image.vkImageUsageFlags;
    info.initialLayout = image.vkInitialLayout;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.flags = vkImageCreateFlags;

    VmaAllocationCreateInfo vmaAllocCreateInfo = {};
    vmaAllocCreateInfo.usage = image.vmaMemoryUsage;
    vmaAllocCreateInfo.flags = image.vmaAllocationCreateFlags;

    VkImage vkImage;
    VmaAllocation vmaAllocation;
    VmaAllocationInfo vmaAllocationInfo;

    auto result = m_vulkanObjs->GetVMA()->CreateImage(
        &info,
        &vmaAllocCreateInfo,
        &vkImage,
        &vmaAllocation,
        &vmaAllocationInfo
    );
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateVkImage: vmaCreateImage failure, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(m_vulkanObjs->GetCalls(),
                 m_vulkanObjs->GetDevice(),
                 VK_OBJECT_TYPE_IMAGE,
                 (uint64_t)vkImage,
                 std::format("Image-{}", image.tag));

    ImageAllocation imageAllocation{
        .vkImage = vkImage,
        .vmaAllocationCreateInfo = vmaAllocCreateInfo,
        .vmaAllocation = vmaAllocation,
        .vmaAllocationInfo = vmaAllocationInfo
    };

    return LoadedImage(image, imageAllocation);
}

bool Images::CreateVkImageView(const ImageView& imageView, LoadedImage& loadedImage) const
{
    if (loadedImage.vkImageViews.contains(imageView.name))
    {
        m_logger->Log(Common::LogLevel::Error,
              "Images::CreateVkImageView: Image already contains an ImageView with the name: {}", imageView.name);
        return false;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = loadedImage.allocation.vkImage;
    viewInfo.viewType = imageView.vkImageViewType;
    viewInfo.format = loadedImage.image.vkFormat;
    viewInfo.subresourceRange.aspectMask = imageView.vkImageAspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = loadedImage.image.numMipLevels;
    viewInfo.subresourceRange.baseArrayLayer = imageView.baseLayer;
    viewInfo.subresourceRange.layerCount = imageView.layerCount;

    VkImageView vkImageView{VK_NULL_HANDLE};

    auto result = m_vulkanObjs->GetCalls()->vkCreateImageView(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        &viewInfo,
        nullptr,
        &vkImageView
    );
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateVkImageView: vkCreateImageView call failed, result code: {}", (uint32_t)result);

        return false;
    }

    SetDebugName(m_vulkanObjs->GetCalls(),
                 m_vulkanObjs->GetDevice(),
                 VK_OBJECT_TYPE_IMAGE_VIEW,
                 (uint64_t)vkImageView,
                 std::format("ImageView-{}-{}", loadedImage.image.tag, imageView.name));

    loadedImage.vkImageViews.insert({imageView.name, vkImageView});

    return true;
}

bool Images::CreateVkImageSampler(const ImageSampler& imageSampler, LoadedImage& loadedImage) const
{
    if (loadedImage.vkSamplers.contains(imageSampler.name))
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateVkImageSampler: Image already contains an ImageSampler with the name: {}", imageSampler.name);
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = imageSampler.vkMagFilter;
    samplerInfo.minFilter = imageSampler.vkMinFilter;
    samplerInfo.addressModeU = imageSampler.vkSamplerAddressModeU;
    samplerInfo.addressModeV = imageSampler.vkSamplerAddressModeV;
    samplerInfo.addressModeW = samplerInfo.addressModeU; // Noteworthy
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = imageSampler.vkSamplerMipmapMode;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // Configure anisotropy if the device supports it
    if (m_vulkanObjs->GetPhysicalDevice()->GetPhysicalDeviceFeatures().samplerAnisotropy == VK_TRUE)
    {
        const auto anisotropyLevel = m_vulkanObjs->GetRenderSettings().textureAnisotropy;

        samplerInfo.anisotropyEnable = anisotropyLevel == TextureAnisotropy::None ? VK_FALSE : VK_TRUE;

        const float maxAnisotropy = anisotropyLevel == TextureAnisotropy::Maximum ?
                                    m_vulkanObjs->GetPhysicalDevice()->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy :
                                    2.0f;

        samplerInfo.maxAnisotropy = maxAnisotropy;
    }

    // Configure mipmap sampling if we have mip levels
    if (loadedImage.image.numMipLevels > 1)
    {
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(loadedImage.image.numMipLevels);
        samplerInfo.mipLodBias = 0.0f;
    }

    VkSampler vkSampler{VK_NULL_HANDLE};

    auto result = m_vulkanObjs->GetCalls()->vkCreateSampler(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        &samplerInfo,
        nullptr,
        &vkSampler
    );
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::CreateVkImageSampler: vkCreateSampler call failed, result code: {}", (uint32_t)result);

        return false;
    }

    SetDebugName(m_vulkanObjs->GetCalls(),
                 m_vulkanObjs->GetDevice(),
                 VK_OBJECT_TYPE_SAMPLER,
                 (uint64_t)vkSampler,
                 std::format("ImageSampler-{}-{}", loadedImage.image.tag, imageSampler.name));

    loadedImage.vkSamplers.insert({imageSampler.name, vkSampler});

    return true;
}

void Images::DestroyImageObjects(const LoadedImage& loadedImage)
{
    for (const auto& vkImageSamplerIt : loadedImage.vkSamplers)
    {
        RemoveDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkImageSamplerIt.second);
        m_vulkanObjs->GetCalls()->vkDestroySampler(m_vulkanObjs->GetDevice()->GetVkDevice(), vkImageSamplerIt.second, nullptr);
    }

    for (const auto& vkImageViewIt : loadedImage.vkImageViews)
    {
        RemoveDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkImageViewIt.second);
        m_vulkanObjs->GetCalls()->vkDestroyImageView(m_vulkanObjs->GetDevice()->GetVkDevice(), vkImageViewIt.second, nullptr);
    }

    RemoveDebugName(m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice(), VK_OBJECT_TYPE_IMAGE, (uint64_t)loadedImage.allocation.vkImage);
    m_vulkanObjs->GetVMA()->DestroyImage(loadedImage.allocation.vkImage, loadedImage.allocation.vmaAllocation);

    // Return the id to the pool now that it's fully no longer in use
    m_imageIds.ReturnId(loadedImage.id);
}

bool Images::DoesImageFormatSupportMipMapGeneration(const VkFormat& vkFormat) const
{
    auto vulkanFuncs = VulkanFuncs(m_logger, m_vulkanObjs);

    const VkFormatProperties vkFormatProperties = vulkanFuncs.GetVkFormatProperties(vkFormat);

    return vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
}

bool Images::TransferImageData(const LoadedImage& loadedImage,
                               const Common::ImageData::Ptr& data,
                               bool isInitialDataTransfer,
                               std::promise<bool> resultPromise)
{
    m_logger->Log(Common::LogLevel::Debug,
      "Images: Starting data transfer for image: {}", loadedImage.id.id);

    // If we're already actively transferring data to the image, error out
    if (m_imagesLoading.contains(loadedImage.id))
    {
        m_logger->Log(Common::LogLevel::Error,
          "Images::TransferImageData: A data transfer for the image is already in progress: {}", loadedImage.id.id);
        return ErrorResult(resultPromise);
    }

    //
    // Determine if we need to generate image mip levels
    //
    bool generateMipMaps = false;
    const auto mipLevels = loadedImage.image.numMipLevels;

    const bool generateMipMapsRequested = mipLevels > 1;

    if (generateMipMapsRequested)
    {
        const bool formatSupportsMipMaps = DoesImageFormatSupportMipMapGeneration(loadedImage.image.vkFormat);
        const bool imageSupportsMipMaps = loadedImage.image.numLayers == 1;

        generateMipMaps = formatSupportsMipMaps && imageSupportsMipMaps;

        if (!generateMipMaps)
        {
            m_logger->Log(Common::LogLevel::Warning,
              "TransferImageData: Provided mipmap count > 1, but device or image format doesn't support mipmaps, ignoring");
        }
    }

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    return vulkanFuncs.QueueSubmit<bool>(
        std::format("TransferImageData-{}", loadedImage.id.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [&](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)  {

            // Mark the image as loading
            m_imagesLoading.insert(loadedImage.id);

            SyncMetrics();

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
                data,
                loadedImage.allocation.vkImage,
                mipLevels,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                finalLayout
            );
            if (!transferResult)
            {
                m_logger->Log(Common::LogLevel::Error, "Images::TransferImageData: Failed to transfer data to GPU image");
                return false;
            }

            //
            // If requested, generate mip maps for the image's other mip levels
            //
            if (generateMipMaps)
            {
                vulkanFuncs.GenerateMipMaps(
                    commandBuffer->GetVkCommandBuffer(),
                    loadedImage.image.size,
                    loadedImage.allocation.vkImage,
                    mipLevels,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );
            }

            return true;
        },
        [=,this](bool commandsSuccessful){
            return OnImageTransferFinished(commandsSuccessful, loadedImage, isInitialDataTransfer);
        },
        std::move(resultPromise),
        EnqueueType::Frameless
    );
}

bool Images::OnImageTransferFinished(bool commandsSuccessful, const LoadedImage& loadedImage, bool isInitialDataTransfer)
{
    m_logger->Log(Common::LogLevel::Debug, "Images: Image transfer finished for image: {}", loadedImage.id.id);

    // Mark the image as no longer loading
    m_imagesLoading.erase(loadedImage.id);

    // Now that the transfer is finished, we want to destroy the image in two cases:
    // 1) While the transfer was happening, we received a call to destroy the image
    // 2) The transfer was an initial data transfer, which failed
    //
    // Note that for update transfers, we're (currently) allowing the image to still
    // exist, even though updating its data failed.
    if (m_imagesToDestroy.contains(loadedImage.id) || (isInitialDataTransfer && !commandsSuccessful))
    {
        m_logger->Log(Common::LogLevel::Debug,
          "Images::OnImageTransferFinished: Image should be destroyed: {}", loadedImage.id.id);

        // Erase our records of the image
        m_images.erase(loadedImage.id);
        m_imagesToDestroy.erase(loadedImage.id);

        // Enqueue image object destruction
        m_postExecutionOps->Enqueue_Current([=, this]() { DestroyImageObjects(loadedImage); });

        SyncMetrics();

        return false;
    }

    SyncMetrics();

    return true;
}

std::optional<LoadedImage> Images::GetImage(ImageId imageId) const
{
    const auto it = m_images.find(imageId);
    if (it == m_images.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void Images::DestroyImage(ImageId imageId, bool destroyImmediately)
{
    const auto it = m_images.find(imageId);
    if (it == m_images.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "Images: Asked to destroy image which doesn't exist: {}", imageId.id);
        return;
    }

    const auto loadedImage = it->second;

    // Whether destroying the image's objects immediately or not below, erase our knowledge
    // of the image; no future render work is allowed to use it
    m_images.erase(it);
    m_imagesToDestroy.erase(imageId);

    // If an image's data transfer is still happening, need to wait until the transfer has finished before
    // destroying the image's Vulkan objects. Mark the image as to be deleted and bail out.
    if (m_imagesLoading.contains(imageId) && !destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Images: Postponing destroy of image: {}", imageId.id);
        m_imagesToDestroy.insert(imageId);
        return;
    }
    else if (destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Images: Destroying image immediately: {}", imageId.id);
        DestroyImageObjects(loadedImage);
    }
    else
    {
        m_logger->Log(Common::LogLevel::Debug, "Images: Enqueueing image destroy: {}", imageId.id);
        m_postExecutionOps->Enqueue_Current([=,this]() { DestroyImageObjects(loadedImage); });
    }

    SyncMetrics();
}

void Images::SyncMetrics() const
{
    m_metrics->SetCounterValue(Renderer_Images_Count, m_images.size());
    m_metrics->SetCounterValue(Renderer_Images_Loading_Count, m_imagesLoading.size());
    m_metrics->SetCounterValue(Renderer_Images_ToDestroy_Count, m_imagesToDestroy.size());
}

}
