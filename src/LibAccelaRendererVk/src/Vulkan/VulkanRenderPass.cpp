/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanRenderPass.h"
#include "VulkanDebug.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"

#include <Accela/Render/IVulkanCalls.h>

#include <algorithm>

namespace Accela::Render
{

VulkanRenderPass::VulkanRenderPass(Common::ILogger::Ptr logger,
                                   IVulkanCallsPtr vk,
                                   VulkanPhysicalDevicePtr physicalDevice,
                                   VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_physicalDevice(std::move(physicalDevice))
    , m_device(std::move(device))
{

}

bool VulkanRenderPass::Create(const std::vector<Attachment>& attachments,
                              const std::vector<Subpass>& subpasses,
                              const std::vector<VkSubpassDependency>& vkDependencies,
                              const std::optional<std::vector<uint32_t>>& multiViewMasks,
                              const std::optional<uint32_t>& multiViewCorrelationMask,
                              const std::string& tag)
{
    //
    // Process attachments
    //
    std::vector<VkAttachmentDescription> vkAttachmentDescriptions;

    for (const auto& attachment : attachments)
    {
        vkAttachmentDescriptions.push_back(attachment.description);
    }

    //
    // Process subpasses
    //
    std::vector<VkSubpassDescription> vkSubpassDescriptions;

    for (const auto& subpass : subpasses)
    {
        //
        // Create a description of the subpass
        //
        VkSubpassDescription vkSubpassDescription{};
        vkSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vkSubpassDescription.colorAttachmentCount = subpass.colorAttachmentRefs.size();
        vkSubpassDescription.pColorAttachments = subpass.colorAttachmentRefs.data();
        if (subpass.depthAttachmentRef.has_value())
        {
            vkSubpassDescription.pDepthStencilAttachment = &subpass.depthAttachmentRef.value();
        }
        vkSubpassDescription.inputAttachmentCount = subpass.inputAttachmentRefs.size();
        vkSubpassDescription.pInputAttachments = subpass.inputAttachmentRefs.data();

        vkSubpassDescriptions.push_back(vkSubpassDescription);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = vkAttachmentDescriptions.size();
    renderPassInfo.pAttachments = vkAttachmentDescriptions.data();
    renderPassInfo.subpassCount = vkSubpassDescriptions.size();
    renderPassInfo.pSubpasses = vkSubpassDescriptions.data();
    renderPassInfo.dependencyCount = vkDependencies.size();
    renderPassInfo.pDependencies = vkDependencies.data();

    VkRenderPassMultiviewCreateInfo vkRenderPassMultiviewCreateInfo{};
    vkRenderPassMultiviewCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
    vkRenderPassMultiviewCreateInfo.subpassCount = subpasses.size();
    vkRenderPassMultiviewCreateInfo.pViewMasks = nullptr;
    vkRenderPassMultiviewCreateInfo.correlationMaskCount = 0;
    vkRenderPassMultiviewCreateInfo.pCorrelationMasks = nullptr;

    if (multiViewMasks || multiViewCorrelationMask)
    {
        if (multiViewMasks)
        {
            vkRenderPassMultiviewCreateInfo.pViewMasks = multiViewMasks->data();
        }

        if (multiViewCorrelationMask)
        {
            vkRenderPassMultiviewCreateInfo.correlationMaskCount = 1;
            vkRenderPassMultiviewCreateInfo.pCorrelationMasks = &multiViewCorrelationMask.value();
        }

        renderPassInfo.pNext = &vkRenderPassMultiviewCreateInfo;
    }

    auto result = m_vk->vkCreateRenderPass(m_device->GetVkDevice(), &renderPassInfo, nullptr, &m_vkRenderPass);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "vkCreateRenderPass call failure, result code: {}", (uint32_t)result);
        return false;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)m_vkRenderPass, "RenderPass-" + tag);

    //
    // Update state
    //
    m_attachments = attachments;
    m_subpasses = subpasses;

    return true;
}

void VulkanRenderPass::Destroy()
{
    if (m_vkRenderPass != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)m_vkRenderPass);

        m_vk->vkDestroyRenderPass(m_device->GetVkDevice(), m_vkRenderPass, nullptr);
    }

    m_vkRenderPass = VK_NULL_HANDLE;
}

bool VulkanRenderPass::HasDepthAttachment() const
{
    return std::ranges::any_of(m_attachments, [](const auto& attachment) {
        return attachment.type == AttachmentType::Depth;
    });
}

std::vector<VkImageLayout> VulkanRenderPass::GetAttachmentInitialLayouts() const
{
    std::vector<VkImageLayout> initialLayouts(m_attachments.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    for (unsigned int x = 0; x < m_attachments.size(); ++x)
    {
        initialLayouts[x] = m_attachments[x].description.initialLayout;
    }

    return initialLayouts;
}

std::vector<VkImageLayout> VulkanRenderPass::GetAttachmentFinalLayouts() const
{
    std::vector<VkImageLayout> finalLayouts(m_attachments.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    for (unsigned int x = 0; x < m_attachments.size(); ++x)
    {
        finalLayouts[x] = m_attachments[x].description.finalLayout;
    }

    return finalLayouts;
}

}
