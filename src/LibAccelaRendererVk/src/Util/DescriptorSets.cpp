#include "DescriptorSets.h"

#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanDescriptorPool.h"

#include <Accela/Render/IVulkanCalls.h>

#include <format>
#include <algorithm>

namespace Accela::Render
{

DescriptorSets::DescriptorSets(Common::ILogger::Ptr logger,
                               IVulkanCallsPtr vk,
                               VulkanDevicePtr device,
                               const VkDescriptorPoolCreateFlags& poolFlags)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
    , m_poolFlags(poolFlags)
{

}

std::optional<VulkanDescriptorSetPtr> DescriptorSets::AllocateDescriptorSet(
    const VulkanDescriptorSetLayoutPtr& layout,
    const std::string& tag)
{
    std::lock_guard<std::mutex> lck(m_poolsMutex);

    // Loop through all untapped pools and try to allocate the set from each
    auto descriptorPool = FetchUntappedDescriptorPool();
    while (descriptorPool != nullptr)
    {
        auto descriptorSet = descriptorPool->AllocateDescriptorSet(layout, tag);
        if (descriptorSet.has_value())
        {
            m_setToPool.insert(std::make_pair(descriptorSet.value(), descriptorPool));
            return descriptorSet.value();
        }

        // If we couldn't allocate the set from this pool, mark the pool as tapped
        MarkPool(descriptorPool, PoolState::Tapped);

        // Fetch the next untapped pool
        descriptorPool = FetchUntappedDescriptorPool();
    }

    // If we've reached this point, we have no untapped pools, so allocate a new pool
    descriptorPool = AllocateNewDescriptorPool(tag);
    if (descriptorPool == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DescriptorSets: Failed to allocate a new descriptor pool for tag: {}", tag);
        return std::nullopt;
    }

    // Keep a record of the new pool
    m_pools.insert(std::make_pair(descriptorPool, PoolState::Untapped));

    m_logger->Log(Common::LogLevel::Debug,
      "DescriptorSets: Allocated a new descriptor pool, total pool of pools size: {}", m_pools.size());

    // Allocate a set from the new pool
    auto descriptorSet = descriptorPool->AllocateDescriptorSet(layout, tag);
    if (descriptorSet.has_value())
    {
        m_setToPool.insert(std::make_pair(descriptorSet.value(), descriptorPool));
        return descriptorSet.value();
    }

    m_logger->Log(Common::LogLevel::Error, "DescriptorSets: All set allocation attempts failed for tag: {}", tag);
    return std::nullopt;
}

bool DescriptorSets::FreeDescriptorSet(const VulkanDescriptorSetPtr& descriptorSet)
{
    if (!(m_poolFlags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
    {
        return false;
    }

    const auto it = m_setToPool.find(descriptorSet);
    if (it == m_setToPool.cend())
    {
        return false;
    }

    // Tell the set's pool to free the set
    it->second->FreeDescriptorSet(descriptorSet);

    // Optimistically return the set's pool back to the list of untapped pools to allow for
    // a new set to be allocated from it now that we just freed one from it
    MarkPool(it->second, PoolState::Untapped);

    // Delete our knowledge of the set now that it's been freed
    m_setToPool.erase(descriptorSet);

    return true;
}

std::optional<VulkanDescriptorSetPtr>
DescriptorSets::CachedAllocateDescriptorSet(const VulkanDescriptorSetLayoutPtr& layout, const std::string& tag)
{
    const auto it = m_cachedDescriptorSets.find(layout);
    if (it != m_cachedDescriptorSets.cend())
    {
        for (auto& cachedDescriptorSet : it->second)
        {
            if (!cachedDescriptorSet.inUse)
            {
                cachedDescriptorSet.inUse = true;
                return cachedDescriptorSet.descriptorSet;
            }
        }
    }

    unsigned int dsIndex = 0;

    if (it != m_cachedDescriptorSets.cend())
    {
        dsIndex = it->second.size();
    }

    const auto allocatedDescriptorSet = AllocateDescriptorSet(layout, std::format("{}-{}", tag, dsIndex));
    if (!allocatedDescriptorSet)
    {
        return std::nullopt;
    }

    m_logger->Log(Common::LogLevel::Debug, "DescriptorSets: Allocated new cached descriptor set: {}-{}", tag, dsIndex);

    CachedDescriptorSet cachedDescriptorSet{};
    cachedDescriptorSet.inUse = true;
    cachedDescriptorSet.descriptorSet = *allocatedDescriptorSet;

    if (it != m_cachedDescriptorSets.cend())
    {
        it->second.push_back(cachedDescriptorSet);
    }
    else
    {
        m_cachedDescriptorSets.insert(std::make_pair(layout, std::vector<CachedDescriptorSet>{cachedDescriptorSet}));
    }

    return *allocatedDescriptorSet;
}

void DescriptorSets::MarkCachedSetsNotInUse()
{
    for (auto& it : m_cachedDescriptorSets)
    {
        for (auto& cachedDescriptorSet : it.second)
        {
            cachedDescriptorSet.inUse = false;
        }
    }
}

void DescriptorSets::ResetAllPools()
{
    std::lock_guard<std::mutex> lck(m_poolsMutex);

    for (auto& it : m_pools)
    {
        it.first->ResetPool();
        it.second = PoolState::Untapped;
    }

    m_setToPool.clear();
    m_cachedDescriptorSets.clear();
}

void DescriptorSets::Destroy()
{
    std::lock_guard<std::mutex> lck(m_poolsMutex);

    for (auto& it : m_pools)
    {
        it.first->Destroy();
    }

    m_pools.clear();
    m_setToPool.clear();
    m_cachedDescriptorSets.clear();
}

VulkanDescriptorPoolPtr DescriptorSets::FetchUntappedDescriptorPool()
{
    const auto it = std::find_if(m_pools.begin(), m_pools.end(), [](const auto& poolIt) {
        return poolIt.second == PoolState::Untapped;
    });
    if (it != m_pools.cend())
    {
        return it->first;
    }

    return nullptr;
}

VulkanDescriptorPoolPtr DescriptorSets::AllocateNewDescriptorPool(const std::string& tag) const
{
    auto descriptorPool = std::make_shared<VulkanDescriptorPool>(m_logger, m_vk, m_device);
    if (!descriptorPool->Create(
        100,   // maxSets
        {
            VulkanDescriptorPool::DescriptorLimit(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100),
            VulkanDescriptorPool::DescriptorLimit(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100),
            VulkanDescriptorPool::DescriptorLimit(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
        },
        m_poolFlags,
        tag
    ))
    {
        m_logger->Log(Common::LogLevel::Error, "DescriptorSets: Unable to allocate new descriptor pool");
        return nullptr;
    }

    return descriptorPool;
}

void DescriptorSets::MarkPool(const VulkanDescriptorPoolPtr& pool, const PoolState& state)
{
    const auto it = m_pools.find(pool);
    if (it != m_pools.cend())
    {
        it->second = state;
    }
}

}
