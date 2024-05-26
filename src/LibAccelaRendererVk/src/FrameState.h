#ifndef LIBACCELARENDERERVK_SRC_FRAMESTATE_H
#define LIBACCELARENDERERVK_SRC_FRAMESTATE_H

#include "ForwardDeclares.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    class FrameState
    {
        public:

            FrameState(Common::ILogger::Ptr logger,
                       Ids::Ptr ids,
                       VulkanObjsPtr vulkanObjs,
                       ITexturesPtr textures,
                       uint8_t frameIndex);

            bool Initialize(const RenderSettings& renderSettings);
            void Destroy();

            [[nodiscard]] uint8_t GetFrameIndex() const noexcept { return m_frameIndex; }
            [[nodiscard]] VulkanCommandPoolPtr GetGraphicsCommandPool() const noexcept { return m_graphicsCommandPool; }
            [[nodiscard]] VulkanCommandBufferPtr GetGraphicsCommandBuffer() const noexcept { return m_graphicsCommandBuffer; }
            [[nodiscard]] VkSemaphore GetImageAvailableSemaphore() const noexcept { return m_imageAvailableSemaphore; }
            [[nodiscard]] VkSemaphore GetRenderFinishedSemaphore() const noexcept { return m_renderFinishedSemaphore; }
            [[nodiscard]] VkFence GetPipelineFence() const noexcept { return m_pipelineFence; }

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            VulkanObjsPtr m_vulkanObjs;
            ITexturesPtr m_textures;

            uint8_t m_frameIndex;

            VulkanCommandPoolPtr m_graphicsCommandPool;
            VulkanCommandBufferPtr m_graphicsCommandBuffer;

            // Semaphore triggered when the frame's swap chain image is ready to be rendered to
            VkSemaphore m_imageAvailableSemaphore{VK_NULL_HANDLE};
            // Semaphore triggered when the frame's render work has finished
            VkSemaphore m_renderFinishedSemaphore{VK_NULL_HANDLE};
            // Fence triggered when the pipeline has finished this frame's work
            VkFence m_pipelineFence{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMESTATE_H
