/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANDESCRIPTORPOOL_H
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANDESCRIPTORPOOL_H

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <optional>

namespace Accela::Render
{
    /**
     * Wrapper for working with Vulkan descriptor pools
     */
    class VulkanDescriptorPool
    {
        public:

            /**
             * Helper struct for defining how many instances of a given
             * descriptor type are allowed
             */
            struct DescriptorLimit
            {
                /**
                 * @param _type Type of descriptor object
                 * @param _descriptorCount How many of the descriptor object type can be created
                 */
                DescriptorLimit(VkDescriptorType _type, uint32_t _descriptorCount)
                    : type(_type)
                    , descriptorCount(_descriptorCount)
                { }

                VkDescriptorType type;
                uint32_t descriptorCount;
            };

        public:

            VulkanDescriptorPool(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device);

            /**
             * Create a new descriptor pool
             *
             * @param maxSets The maximum number of descriptor sets that can be allocated from this pool
             * @param maxDescriptorSets Limits for the maximum number of descriptors that can be allocated from this pool
             * @param flags Flags that describe how the descriptor pool should operate
             * @param tag A debug name to associate with this descriptor pool
             *
             * @return Whether the pool was created successfully
             */
            bool Create(
                const uint32_t& maxDescriptorSets,
                const std::vector<DescriptorLimit>& maxDescriptors,
                const VkDescriptorPoolCreateFlags& flags,
                const std::string& tag);

            /**
             * Destroys this pool and frees any resources associated with the pool or outstanding descriptor
             * sets created from it
             */
            void Destroy();

            /**
             * Allocate a descriptor set from this pool
             *
             * @param layout The layout that the descriptor set should have
             * @param tag A debug name to associate with the descriptor set
             *
             * @return The created VulkanDescriptorSet, or std::nullopt on error
             */
            std::optional<VulkanDescriptorSetPtr> AllocateDescriptorSet(
                const VulkanDescriptorSetLayoutPtr& layout,
                const std::string& tag);

            /**
             * Free the specified descriptor set, reclaiming its memory. This pool must have
             * been created with the VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag.
             *
             * @param descriptorSet The descriptor set to be freed
             */
            void FreeDescriptorSet(const VulkanDescriptorSetPtr& descriptorSet);

            /**
             * Frees all descriptor sets currently allocated from this pool
             */
            void ResetPool();

            /**
             * @return The VkDescriptorPool object for this pool
             */
            [[nodiscard]] VkDescriptorPool GetVkDescriptorPool() const { return m_vkDescriptorPool; }

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;

            VkDescriptorPool m_vkDescriptorPool{VK_NULL_HANDLE};
            VkDescriptorPoolCreateFlags m_createFlags{0};

            std::set<VulkanDescriptorSetPtr> m_allocatedSets;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANDESCRIPTORPOOL_H
