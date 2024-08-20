/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanCommandPool.h"
#include "VulkanDebug.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

VulkanCommandPool::VulkanCommandPool(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
{

}

bool VulkanCommandPool::Create(
    const uint32_t& queueFamilyIndex,
    const VkCommandPoolCreateFlags& flags,
    const std::string& tag)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    const auto result = m_vk->vkCreateCommandPool(m_device->GetVkDevice(), &poolInfo, nullptr, &m_vkCommandPool);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "vkCreateCommandPool call failure, result code: {}", (uint32_t)result);
        return false;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_vkCommandPool, "CommandPool-" + tag);

    m_createFlags = flags;

    return true;
}

void VulkanCommandPool::Destroy()
{
    if (m_vkCommandPool == VK_NULL_HANDLE)
    {
        return;
    }

    RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_vkCommandPool);

    // Freeing command buffers individually rather than just relying on vkDestroyCommandPool call
    // below to free them in order to reclaim memory tied to the debug name of each command buffer
    while (!m_allocatedBuffers.empty())
    {
        FreeCommandBuffer(*(m_allocatedBuffers.begin()));
    }

    m_vk->vkDestroyCommandPool(m_device->GetVkDevice(), m_vkCommandPool, nullptr);
    m_vkCommandPool = VK_NULL_HANDLE;
    m_createFlags = 0;
}

std::optional<VulkanCommandBufferPtr> VulkanCommandPool::AllocateCommandBuffer(VulkanCommandPool::CommandBufferType type, const std::string& tag)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_vkCommandPool;
    allocInfo.commandBufferCount = 1;

    switch (type)
    {
        case CommandBufferType::Primary:
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        break;
        case CommandBufferType::Secondary:
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        break;
    }

    VkCommandBuffer vkCommandBuffer{VK_NULL_HANDLE};

    const auto result = m_vk->vkAllocateCommandBuffers(m_device->GetVkDevice(), &allocInfo, &vkCommandBuffer);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "vkAllocateCommandBuffers call failure, result code: {}", (uint32_t)result);
        return std::nullopt;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)vkCommandBuffer, "CommandBuffer-" + tag);

    auto commandBuffer = std::make_shared<VulkanCommandBuffer>(m_logger, m_vk, m_device, vkCommandBuffer);
    m_allocatedBuffers.insert(commandBuffer);

    return commandBuffer;
}

void VulkanCommandPool::FreeCommandBuffer(const VulkanCommandBufferPtr& commandBuffer)
{
    const auto vkCommandBuffer = commandBuffer->GetVkCommandBuffer();

    const auto it = m_allocatedBuffers.find(commandBuffer);
    if (it != m_allocatedBuffers.cend())
    {
        RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)vkCommandBuffer);
        m_vk->vkFreeCommandBuffers(m_device->GetVkDevice(), m_vkCommandPool, 1, &vkCommandBuffer);
        m_allocatedBuffers.erase(it);
    }
}

void VulkanCommandPool::ResetCommandBuffer(const VulkanCommandBufferPtr& commandBuffer, bool trimMemory)
{
    const auto vkCommandBuffer = commandBuffer->GetVkCommandBuffer();

    if (!(m_createFlags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))
    {
        m_logger->Log(Common::LogLevel::Error,"Attempted to reset command buffer in a pool that doesn't support resetting");
        return;
    }

    VkCommandBufferResetFlags flags = 0;
    if (trimMemory)
    {
        flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
    }

    const auto it = m_allocatedBuffers.find(commandBuffer);
    if (it != m_allocatedBuffers.cend())
    {
        m_vk->vkResetCommandBuffer(vkCommandBuffer, flags);
    }
}

void VulkanCommandPool::ResetPool(bool trimMemory)
{
    VkCommandPoolResetFlags flags = 0;
    if (trimMemory)
    {
        flags = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;
    }

    m_vk->vkResetCommandPool(m_device->GetVkDevice(), m_vkCommandPool, flags);
}

}
