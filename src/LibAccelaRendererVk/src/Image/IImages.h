/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_IIMAGES_H
#define LIBACCELARENDERERVK_SRC_IMAGE_IIMAGES_H

#include "ImageDefinition.h"
#include "LoadedImage.h"

#include "../ForwardDeclares.h"
#include "../InternalId.h"

#include <Accela/Common/ImageData.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <expected>
#include <future>
#include <optional>

namespace Accela::Render
{
    class IImages
    {
        public:

            virtual ~IImages() = default;

            virtual bool Initialize(VulkanCommandPoolPtr transferCommandPool, VkQueue vkTransferQueue) = 0;
            virtual void Destroy() = 0;

            [[nodiscard]] virtual std::expected<ImageId, bool> CreateEmptyImage(const ImageDefinition& imageDefinition) = 0;

            [[nodiscard]] virtual std::expected<ImageId, bool> CreateFilledImage(const ImageDefinition& imageDefinition,
                                                                                 const Common::ImageData::Ptr& data,
                                                                                 std::promise<bool> resultPromise) = 0;

            [[nodiscard]] virtual std::optional<LoadedImage> GetImage(ImageId imageId) const = 0;

            virtual void DestroyImage(ImageId imageId, bool destroyImmediately) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_IIMAGES_H
