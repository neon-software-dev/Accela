/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDescriptorPool.h"
#include "VulkanDebug.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDescriptorSetLayout.h"

#include <Accela/Render/IVulkanCalls.h>

#include <algorithm>

namespace Accela::Render
{

VulkanDescriptorPool::VulkanDescriptorPool(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
{

}

bool VulkanDescriptorPool::Create(
    const uint32_t& maxDescriptorSets,
    const std::vector<DescriptorLimit>& descriptorLimits,
    const VkDescriptorPoolCreateFlags& flags,
    const std::string& tag)
{
    std::vector<VkDescriptorPoolSize> poolSizes;

    std::transform(descriptorLimits.begin(), descriptorLimits.end(), std::back_inserter(poolSizes), [](const auto& descriptorLimit){
        VkDescriptorPoolSize poolSize{};
        poolSize.type = descriptorLimit.type;
        poolSize.descriptorCount = descriptorLimit.descriptorCount;
        return poolSize;
    });

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxDescriptorSets;
    poolInfo.flags = flags;

    const auto result = m_vk->vkCreateDescriptorPool(m_device->GetVkDevice(), &poolInfo, nullptr, &m_vkDescriptorPool);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "vkCreateDescriptorPool call failure, result code: {}", (uint32_t)result);
        return false;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)m_vkDescriptorPool, "DescriptorPool-" + tag);

    m_createFlags = flags;

    return true;
}

void VulkanDescriptorPool::Destroy()
{
    if (m_vkDescriptorPool == VK_NULL_HANDLE)
    {
        return;
    }

    // Freeing descriptor sets individually rather than just relying on vkDestroyDescriptorPool call
    // below to free them in order to reclaim memory tied to the debug name of each command buffer
    while (!m_allocatedSets.empty())
    {
        const auto descriptorSetIt = m_allocatedSets.begin();

        // Only actually call vkFreeDescriptorSets if the pool supports it
        if (m_createFlags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        {
            FreeDescriptorSet(*descriptorSetIt);
        }
        // Otherwise, just remove the debug name before destroying the pool
        else
        {
            RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)(*descriptorSetIt)->GetVkDescriptorSet());
            m_allocatedSets.erase(descriptorSetIt);
        }
    }

    m_vk->vkDestroyDescriptorPool(m_device->GetVkDevice(), m_vkDescriptorPool, nullptr);

    m_vkDescriptorPool = VK_NULL_HANDLE;
    m_createFlags = 0;
}

std::optional<VulkanDescriptorSetPtr> VulkanDescriptorPool::AllocateDescriptorSet(
    const VulkanDescriptorSetLayoutPtr& layout,
    const std::string& tag)
{
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.push_back(layout->GetVkDescriptorSetLayout());

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_vkDescriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    VkDescriptorSet vkDescriptorSet{VK_NULL_HANDLE};

    const auto result = m_vk->vkAllocateDescriptorSets(m_device->GetVkDevice(), &allocInfo, &vkDescriptorSet);
    if (result != VK_SUCCESS)
    {
        // Don't log errors about out of memory pools as by design we run pools out of memory then create more when needed
        if (result != VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            m_logger->Log(Common::LogLevel::Error, "vkAllocateDescriptorSets failure, result code: {}", (uint32_t)result);
        }
        return std::nullopt;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vkDescriptorSet, "DescriptorSet-" + tag);

    auto descriptorSet = std::make_shared<VulkanDescriptorSet>(m_logger, m_vk, m_device, vkDescriptorSet);

    m_allocatedSets.insert(descriptorSet);

    return descriptorSet;
}

void VulkanDescriptorPool::FreeDescriptorSet(const VulkanDescriptorSetPtr& descriptorSet)
{
    if (!(m_createFlags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
    {
        m_logger->Log(Common::LogLevel::Error, "Attempted to free a descriptor set in a pool that doesn't support it");
        return;
    }

    VkDescriptorSet vkDescriptorSet = descriptorSet->GetVkDescriptorSet();

    const auto it = m_allocatedSets.find(descriptorSet);
    if (it != m_allocatedSets.cend())
    {
        RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) vkDescriptorSet);
        m_vk->vkFreeDescriptorSets(m_device->GetVkDevice(), m_vkDescriptorPool, 1, &vkDescriptorSet);
        m_allocatedSets.erase(it);
    }
}

void VulkanDescriptorPool::ResetPool()
{
    m_vk->vkResetDescriptorPool(m_device->GetVkDevice(), m_vkDescriptorPool, 0);
    m_allocatedSets.clear();
}

}
