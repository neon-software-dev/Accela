/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERSTATE_H
#define LIBACCELARENDERERVK_SRC_RENDERSTATE_H

#include "RenderOperation.h"

#include "ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <optional>

namespace Accela::Render
{
    class RenderState
    {
        public:

            RenderState(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls);

            void PrepareOperation(const VulkanCommandBufferPtr& commandBuffer, const RenderOperation& renderOperation);

            void Destroy();

        private:

            struct ImageState
            {
                VkImageLayout currentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
                std::optional<ImageAccess> currentAccess;
            };

        private:

            void PrepareImageAccess(const VulkanCommandBufferPtr& commandBuffer, VkImage vkImage, const ImageAccess& imageAccess);
            void PrepareImageAccess(const VulkanCommandBufferPtr& commandBuffer, VkImage vkImage, const ImageAccess& imageAccess, ImageState& currentState);

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vulkanCalls;

            std::unordered_map<VkImage, ImageState> m_imageStates;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERSTATE_H
