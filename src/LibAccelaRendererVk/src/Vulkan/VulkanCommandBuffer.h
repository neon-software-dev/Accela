/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMANDBUFFER
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMANDBUFFER

#include "../ForwardDeclares.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace Accela::Render
{
    class VulkanCommandBuffer
    {
        public:

            VulkanCommandBuffer(Common::ILogger::Ptr logger,
                                IVulkanCallsPtr vk,
                                VulkanDevicePtr device,
                                VkCommandBuffer vkCommandBuffer);

            void Begin(const VkCommandBufferUsageFlagBits& flags) const;
            void End() const;

            void CmdBeginRenderPass(const VulkanRenderPassPtr& renderPass,
                                    const VulkanFramebufferPtr& framebuffer,
                                    const VkSubpassContents& vkSubpassContents,
                                    const std::vector<VkClearValue>& vkAttachmentClearValues) const;
            void CmdNextSubpass() const;
            void CmdEndRenderPass() const;

            void CmdBindPipeline(const VulkanPipelinePtr& pipeline) const;

            void CmdBindVertexBuffers(const uint32_t& firstBinding,
                                      const uint32_t& bindingCount,
                                      const std::vector<VkBuffer>& buffers,
                                      const std::vector<VkDeviceSize>& pOffsets) const;

            void CmdBindIndexBuffer(const VkBuffer& buffer,
                                    const VkDeviceSize& offset,
                                    const VkIndexType& indexType) const;

            void CmdBindDescriptorSets(const VulkanPipelinePtr& pipeline,
                                       const uint32_t& firstSetNumber,
                                       const std::vector<VkDescriptorSet>& descriptorSets) const;

            void CmdDraw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertex,
                         uint32_t firstInstance) const;

            void CmdDrawIndexed(const uint32_t& indexCount,
                                const uint32_t& instanceCount,
                                const uint32_t& firstIndex,
                                const int32_t& vertexOffset,
                                const uint32_t& firstInstance) const;

            void CmdSetViewport(const Viewport& viewport, float minDepth, float maxDepth) const;

            void CmdPushConstants(const VulkanPipelinePtr& pipeline,
                                  VkShaderStageFlags stageFlags,
                                  uint32_t offset,
                                  uint32_t size,
                                  const void* pValues) const;

            void CmdClearAttachments(const std::vector<VkClearAttachment>& vkClearAttachments,
                                     const std::vector<VkClearRect>& vkClearRects) const;


            /**
             * @return The VkCommandBuffer object for this command buffer
             */
            [[nodiscard]] VkCommandBuffer GetVkCommandBuffer() const { return m_vkCommandBuffer; }

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;
            VkCommandBuffer m_vkCommandBuffer{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMANDBUFFER
