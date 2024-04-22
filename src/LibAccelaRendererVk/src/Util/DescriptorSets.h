/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERVK_SRC_UTIL_DESCRIPTORSETS
#define LIBACCELARENDERVK_SRC_UTIL_DESCRIPTORSETS

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <mutex>
#include <unordered_map>
#include <optional>
#include <vector>

namespace Accela::Render
{
    /**
     * Provides allocating descriptor sets from a pool of descriptor pools
     */
    class DescriptorSets
    {
        public:

            DescriptorSets(Common::ILogger::Ptr logger,
                           IVulkanCallsPtr vk,
                           VulkanDevicePtr device,
                           const VkDescriptorPoolCreateFlags& poolFlags);

            /**
             * Allocate a new descriptor set with the given layout.
             *
             * @param layout The layout for the descriptor set
             * @param tag A debug tag to associate with the request
             *
             * @return The allocated descriptor set, or std::nullopt on error
             */
            [[nodiscard]] std::optional<VulkanDescriptorSetPtr> AllocateDescriptorSet(
                const VulkanDescriptorSetLayoutPtr& layout,
                const std::string& tag);

            /**
             * Free the specified descriptor set that was previously allocated by this
             * object. Note: poolFlags provided when constructing this object must have
             * included VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
             *
             * @param descriptorSet The descriptor set to free
             */
            bool FreeDescriptorSet(const VulkanDescriptorSetPtr& descriptorSet);

            /**
             * Same as AllocateDescriptorSet, but internally maintains a collection of
             * cached DescriptorSets for a given layout, and will return from that list
             * of cached before allocating a new set (and adding it to the cache). If
             * returning a cached set, will mark that cache as now "in use" and it will
             * not be returned again on subsequent calls until MarkCachedSetsNotInUse is
             * called.
             *
             * @param layout The layout for the descriptor set
             * @param tag A debug tag to associate with the request
             *
             * @return The descriptor set, or std::nullopt on error
             */
            [[nodiscard]] std::optional<VulkanDescriptorSetPtr> CachedAllocateDescriptorSet(
                const VulkanDescriptorSetLayoutPtr& layout,
                const std::string& tag);

            /**
             * Call this to mark all previously cached descriptor sets as no longer in use,
             * allowing them to be returned from CachedAllocateDescriptorSet again.
             */
            void MarkCachedSetsNotInUse();

            /**
             * Reset all descriptor pools, returning all their memory
             */
            void ResetAllPools();

            /**
             * Destroy all descriptor pools
             */
            void Destroy();

        private:

            enum PoolState
            {
                Tapped,     // A previous call to this pool to allocate failed, it has no memory left
                Untapped    // We should still attempt to allocate from this pool
            };

            struct CachedDescriptorSet
            {
                bool inUse{true};
                VulkanDescriptorSetPtr descriptorSet;
            };

        private:

            [[nodiscard]] VulkanDescriptorPoolPtr FetchUntappedDescriptorPool();
            [[nodiscard]] VulkanDescriptorPoolPtr AllocateNewDescriptorPool(const std::string& tag) const;

            void MarkPool(const VulkanDescriptorPoolPtr& pool, const PoolState& state);

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;
            VkDescriptorPoolCreateFlags m_poolFlags;

            std::unordered_map<VulkanDescriptorPoolPtr, PoolState> m_pools;
            std::unordered_map<VulkanDescriptorSetPtr, VulkanDescriptorPoolPtr> m_setToPool;
            std::mutex m_poolsMutex;

            std::unordered_map<VulkanDescriptorSetLayoutPtr, std::vector<CachedDescriptorSet>> m_cachedDescriptorSets;
    };
}

#endif //LIBACCELARENDERVK_SRC_UTIL_DESCRIPTORSETS
