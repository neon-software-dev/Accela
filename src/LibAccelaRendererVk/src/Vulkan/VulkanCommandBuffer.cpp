/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

VulkanCommandBuffer::VulkanCommandBuffer(Common::ILogger::Ptr logger,
                                         IVulkanCallsPtr vk,
                                         VulkanDevicePtr device,
                                         VkCommandBuffer vkCommandBuffer)
     : m_logger(std::move(logger))
     , m_vk(std::move(vk))
     , m_device(std::move(device))
     , m_vkCommandBuffer(vkCommandBuffer)
{

}

void VulkanCommandBuffer::Begin(const VkCommandBufferUsageFlagBits& flags) const
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    auto result = m_vk->vkBeginCommandBuffer(m_vkCommandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanCommandBuffer: vkBeginCommandBuffer call failure");
    }
}

void VulkanCommandBuffer::End() const
{
    const auto result = m_vk->vkEndCommandBuffer(m_vkCommandBuffer);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanCommandBuffer: vkEndCommandBuffer call failure");
    }
}

void VulkanCommandBuffer::CmdBeginRenderPass(const VulkanRenderPassPtr& renderPass,
                                             const VulkanFramebufferPtr& framebuffer,
                                             const VkSubpassContents& vkSubpassContents,
                                             const std::vector<VkClearValue>& vkAttachmentClearValues) const
{
    VkExtent2D passExtent{};
    passExtent.width = framebuffer->GetSize()->w;
    passExtent.height = framebuffer->GetSize()->h;

    VkRenderPassBeginInfo passInfo{};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = renderPass->GetVkRenderPass();
    passInfo.framebuffer = framebuffer->GetVkFramebuffer();
    passInfo.renderArea.offset = {0, 0};
    passInfo.renderArea.extent = passExtent;
    passInfo.clearValueCount = vkAttachmentClearValues.size();
    passInfo.pClearValues = vkAttachmentClearValues.data();

    // Rendering
    m_vk->vkCmdBeginRenderPass(m_vkCommandBuffer, &passInfo, vkSubpassContents);
}

void VulkanCommandBuffer::CmdNextSubpass() const
{
    m_vk->vkCmdNextSubpass(m_vkCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::CmdEndRenderPass() const
{
    m_vk->vkCmdEndRenderPass(m_vkCommandBuffer);
}

void VulkanCommandBuffer::CmdBindPipeline(const VulkanPipelinePtr& pipeline) const
{
    m_vk->vkCmdBindPipeline(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVkPipeline());
}

void VulkanCommandBuffer::CmdBindVertexBuffers(const uint32_t& firstBinding,
                                               const uint32_t& bindingCount,
                                               const std::vector<VkBuffer>& buffers,
                                               const std::vector<VkDeviceSize>& pOffsets) const
{
    m_vk->vkCmdBindVertexBuffers(m_vkCommandBuffer, firstBinding, bindingCount, buffers.data(), pOffsets.data());
}

void VulkanCommandBuffer::CmdBindIndexBuffer(const VkBuffer& buffer,
                                             const VkDeviceSize& offset,
                                             const VkIndexType& indexType) const
{
    m_vk->vkCmdBindIndexBuffer(m_vkCommandBuffer, buffer, offset, indexType);
}

void VulkanCommandBuffer::CmdBindDescriptorSets(const VulkanPipelinePtr& pipeline,
                                                const uint32_t& firstSetNumber,
                                                const std::vector<VkDescriptorSet>& descriptorSets) const
{
    m_vk->vkCmdBindDescriptorSets(
        m_vkCommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->GetVkPipelineLayout(),
        firstSetNumber,                     // First set number
        descriptorSets.size(),              // Number of descriptor sets to bind
        descriptorSets.data(),
        0,                                  // dynamicOffsetCount
        nullptr                             // pDynamicOffsets
    );
}

void VulkanCommandBuffer::CmdDraw(uint32_t vertexCount,
                                  uint32_t instanceCount,
                                  uint32_t firstVertex,
                                  uint32_t firstInstance) const
{
    m_vk->vkCmdDraw(m_vkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::CmdDrawIndexed(const uint32_t& indexCount,
                                         const uint32_t& instanceCount,
                                         const uint32_t& firstIndex,
                                         const int32_t& vertexOffset,
                                         const uint32_t& firstInstance) const
{
    m_vk->vkCmdDrawIndexed(m_vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::CmdSetViewport(const Viewport& viewport, float minDepth, float maxDepth) const
{
    VkViewport vkViewport{};
    vkViewport.x = (float)viewport.x;
    vkViewport.y = (float)viewport.y;
    vkViewport.width = (float)viewport.w;
    vkViewport.height = (float)viewport.h;
    vkViewport.minDepth = minDepth;
    vkViewport.maxDepth = maxDepth;

    m_vk->vkCmdSetViewport(m_vkCommandBuffer, 0, 1, &vkViewport);
}

void VulkanCommandBuffer::CmdPushConstants(const VulkanPipelinePtr& pipeline,
                                           VkShaderStageFlags stageFlags,
                                           uint32_t offset,
                                           uint32_t size,
                                           const void* pValues) const
{
    m_vk->vkCmdPushConstants(m_vkCommandBuffer, pipeline->GetVkPipelineLayout(), stageFlags, offset, size, pValues);
}

void VulkanCommandBuffer::CmdClearAttachments(const std::vector<VkClearAttachment>& vkClearAttachments,
                                              const std::vector<VkClearRect>& vkClearRects) const
{
    vkCmdClearAttachments(m_vkCommandBuffer,
                          vkClearAttachments.size(),
                          vkClearAttachments.data(),
                          vkClearRects.size(),
                          vkClearRects.data());

}

}
