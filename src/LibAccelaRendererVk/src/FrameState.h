/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FRAMESTATE_H
#define LIBACCELARENDERERVK_SRC_FRAMESTATE_H

#include "ForwardDeclares.h"
#include "InternalId.h"

#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    class FrameState
    {
        public:

            FrameState(Common::ILogger::Ptr logger,
                       VulkanObjsPtr vulkanObjs,
                       IRenderTargetsPtr renderTargets,
                       IImagesPtr images,
                       uint8_t frameIndex);

            bool Initialize(const RenderSettings& renderSettings);
            void Destroy();

            [[nodiscard]] uint8_t GetFrameIndex() const noexcept { return m_frameIndex; }
            [[nodiscard]] VulkanCommandPoolPtr GetGraphicsCommandPool() const noexcept { return m_graphicsCommandPool; }
            [[nodiscard]] VulkanCommandBufferPtr GetRenderCommandBuffer() const noexcept { return m_renderCommandBuffer; }
            [[nodiscard]] VulkanCommandBufferPtr GetSwapChainBlitCommandBuffer() const noexcept { return m_swapChainBlitCommandBuffer; }
            [[nodiscard]] VkSemaphore GetImageAvailableSemaphore() const noexcept { return m_imageAvailableSemaphore; }
            [[nodiscard]] VkSemaphore GetRenderFinishedSemaphore() const noexcept { return m_renderFinishedSemaphore; }
            [[nodiscard]] VkSemaphore GetSwapChainBlitFinishedSemaphore() const noexcept { return m_swapChainBlitFinishedSemaphore; }
            [[nodiscard]] VkFence GetPipelineFence() const noexcept { return m_pipelineFence; }
            [[nodiscard]] ImageId GetObjectDetailImageId() const noexcept { return m_objectDetailImageId; }

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;
            IRenderTargetsPtr m_renderTargets;
            ITexturesPtr m_textures;
            IImagesPtr m_images;

            uint8_t m_frameIndex;

            VulkanCommandPoolPtr m_graphicsCommandPool;

            // Holds commands to render a frame
            VulkanCommandBufferPtr m_renderCommandBuffer;
            // Holds commands to blit a rendered frame to the swap chain
            VulkanCommandBufferPtr m_swapChainBlitCommandBuffer;

            // Semaphore triggered when the frame's swap chain image is ready to be rendered to
            VkSemaphore m_imageAvailableSemaphore{VK_NULL_HANDLE};
            // Semaphore triggered when the frame's render work has finished
            VkSemaphore m_renderFinishedSemaphore{VK_NULL_HANDLE};
            // Semaphore triggered when the swap chain blit work has finished
            VkSemaphore m_swapChainBlitFinishedSemaphore{VK_NULL_HANDLE};
            // Fence triggered when the pipeline has finished this frame's work
            VkFence m_pipelineFence{VK_NULL_HANDLE};

            // Image that receives a copy of the object detail render output
            ImageId m_objectDetailImageId;
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMESTATE_H
