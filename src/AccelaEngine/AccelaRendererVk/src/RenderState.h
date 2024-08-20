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
    /**
     * Keeps track of render state which is manipulated via pipeline operations. Currently only used to keep
     * track of state related to images, for synchronization purposes.
     *
     * If kept informed of all image accesses via PrepareOperation calls, it will insert pipeline barriers as
     * needed to properly synchronize access to the images and transition images to new layouts as needed.
     */
    class RenderState
    {
        public:

            RenderState(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls);

            /**
             * Report a render operation as about to happen. Will synchronize resources as needed to
             * fulfill the render operation.
             *
             * @param commandBuffer The command buffer the render operation will take place in
             * @param renderOperation The render operation that's about to occur
             */
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
