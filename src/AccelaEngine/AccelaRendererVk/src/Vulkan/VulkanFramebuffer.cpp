/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanFramebuffer.h"
#include "VulkanDebug.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

VulkanFramebuffer::VulkanFramebuffer(Common::ILogger::Ptr logger,
                                     IVulkanCallsPtr vk,
                                     VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
{

}

bool VulkanFramebuffer::Create(const VulkanRenderPassPtr& compatibleRenderPass,
                               const std::vector<VkImageView>& attachments,
                               const USize& size,
                               const uint32_t& layers,
                               const std::string& tag)
{
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = compatibleRenderPass->GetVkRenderPass();
    framebufferInfo.attachmentCount = attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = size.w;
    framebufferInfo.height = size.h;
    framebufferInfo.layers = layers;

    const auto result = m_vk->vkCreateFramebuffer(m_device->GetVkDevice(), &framebufferInfo, nullptr, &m_vkFrameBuffer);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "vkCreateFramebuffer call failure, result code: {}", (uint32_t)result);
        return false;
    }

    m_attachments = attachments;
    m_size = size;

    SetDebugName(
        m_vk,
        m_device,
        VK_OBJECT_TYPE_FRAMEBUFFER,
        (uint64_t)m_vkFrameBuffer,
        "Framebuffer-" + tag
    );

    return true;
}

void VulkanFramebuffer::Destroy()
{
    if (m_vkFrameBuffer != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_vkFrameBuffer);
        m_vk->vkDestroyFramebuffer(m_device->GetVkDevice(), m_vkFrameBuffer, nullptr);
        m_vkFrameBuffer = VK_NULL_HANDLE;
    }

    m_size = {};
    m_attachments.clear();
}

}
