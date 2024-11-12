/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_IMAGES_H
#define LIBACCELARENDERERVK_SRC_IMAGE_IMAGES_H

#include "IImages.h"

#include "../Util/ImageAllocation.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/IdSource.h>

#include <unordered_map>
#include <unordered_set>

namespace Accela::Render
{
    class Images : public IImages
    {
        public:

            Images(Common::ILogger::Ptr logger,
                   Common::IMetrics::Ptr metrics,
                   VulkanObjsPtr vulkanObjs,
                   IBuffersPtr buffers,
                   PostExecutionOpsPtr postExecutionOps);

            bool Initialize(VulkanCommandPoolPtr transferCommandPool, VkQueue vkTransferQueue) override;
            void Destroy() override;

            [[nodiscard]] std::expected<ImageId, bool> CreateEmptyImage(const ImageDefinition& imageDefinition) override;

            [[nodiscard]] std::expected<ImageId, bool> CreateFilledImage(const ImageDefinition& imageDefinition,
                                                                         const Common::ImageData::Ptr& data,
                                                                         std::promise<bool> resultPromise) override;

            [[nodiscard]] bool UpdateImage(const ImageId& imageId,
                                           const Common::ImageData::Ptr& data,
                                           std::promise<bool> resultPromise) override;

            void RecordImageLayout(const ImageId& imageId, VkImageLayout vkImageLayout) override;

            [[nodiscard]] std::optional<LoadedImage> GetImage(ImageId imageId) const override;

            void DestroyImage(ImageId imageId, bool destroyImmediately) override;

        private:

            [[nodiscard]] std::expected<LoadedImage, bool> CreateImageObjects(const ImageDefinition& imageDefinition);

            [[nodiscard]] std::expected<LoadedImage, bool> CreateVkImage(const Image& image) const;
            [[nodiscard]] bool CreateVkImageView(const ImageView& imageView, LoadedImage& loadedImage) const;
            [[nodiscard]] bool CreateVkImageSampler(const ImageSampler& imageSampler, LoadedImage& loadedImage) const;

            void DestroyImageObjects(const LoadedImage& loadedImage);

            bool DoesImageFormatSupportMipMapGeneration(const VkFormat& vkFormat) const;

            [[nodiscard]] bool TransferImageData(LoadedImage& loadedImage,
                                                 const Common::ImageData::Ptr& data,
                                                 bool isInitialDataTransfer,
                                                 std::promise<bool> resultPromise);

            [[nodiscard]] bool TransferImageData(const IBuffersPtr& buffers,
                                                 const PostExecutionOpsPtr& postExecutionOps,
                                                 const VulkanCommandBufferPtr& commandBuffer,
                                                 VkFence vkExecutionFence,
                                                 const Common::ImageData::Ptr& sourceImageData,
                                                 const LoadedImage& destImage,
                                                 VkImageAspectFlags vkTransferImageAspectFlags,
                                                 VkImageLayout vkCurrentImageLayout,
                                                 VkImageLayout vkFinalImageLayout,
                                                 VkPipelineStageFlags vkEarliestUsageFlags);

            [[nodiscard]] bool OnImageTransferFinished(bool commandsSuccessful,
                                                       const LoadedImage& loadedImage,
                                                       bool isInitialDataTransfer);

            void SyncMetrics() const;

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            VulkanObjsPtr m_vulkanObjs;
            IBuffersPtr m_buffers;
            PostExecutionOpsPtr m_postExecutionOps;

            VulkanCommandPoolPtr m_transferCommandPool;
            VkQueue m_vkTransferQueue{VK_NULL_HANDLE};

            Common::IdSource<ImageId> m_imageIds;

            std::unordered_map<ImageId, LoadedImage> m_images;
            std::unordered_map<ImageId, unsigned int> m_imagesLoading; // ImageId -> Number of active data transfers
            std::unordered_set<ImageId> m_imagesToDestroy;
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_IMAGES_H
