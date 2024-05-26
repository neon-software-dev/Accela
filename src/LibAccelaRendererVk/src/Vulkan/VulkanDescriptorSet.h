/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERVK_SRC_VULKAN_VULKANDESCRIPTORSET_H
#define LIBACCELARENDERVK_SRC_VULKAN_VULKANDESCRIPTORSET_H

#include "VulkanDescriptorSetLayout.h"

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>
#include <optional>

namespace Accela::Render
{
    /**
     * Wrapper for working with descriptor sets
     */
    class VulkanDescriptorSet
    {
        public:

            VulkanDescriptorSet(Common::ILogger::Ptr logger,
                                IVulkanCallsPtr vk,
                                VulkanDevicePtr device,
                                VkDescriptorSet vkDescriptorSet);

            /**
             * @return The VkDescriptorSet object for this descriptor set
             */
            [[nodiscard]] VkDescriptorSet GetVkDescriptorSet() const { return m_vkDescriptorSet; }

            /**
             * Updates the descriptor set to bind a buffer to a binding index
             *
             * @param bindingIndex The binding index to bind to
             * @param vkDescriptorType The type of buffer to be bound
             * @param vkBuffer The buffer to be bound
             * @param bufferByteSize The byte size of the buffer being bound
             */
            void WriteBufferBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                 VkDescriptorType vkDescriptorType,
                                 VkBuffer vkBuffer,
                                 std::size_t offset,
                                 std::size_t bufferByteSize);

            /**
             * Updates the descriptor set to bind a combined Image/Sampler to a binding index
             *
             * @param bindingIndex The binding index to bind to
             * @param vkImageView The image view to be bound
             * @param vkSampler The sampler to be bound
             */
            void WriteCombinedSamplerBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                          VkImageView vkImageView,
                                          VkSampler vkSampler);

            /**
             * Updates the descriptor set to bind an array of Image/Samplers to a binding index
             *
             * @param bindingIndex The binding index to bind to
             * @param samplers The image/samplers to be bound
             */
            void WriteCombinedSamplerBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                          const std::vector<std::pair<VkImageView, VkSampler>>& samplers);

            /**
             * Updates the descriptor set to bind an input attachment Image
             *
             * @param bindingIndex The binding index to bind to
             * @param vkImageView The image view to be bound
             */
            void WriteInputAttachmentBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                          VkImageView vkImageView);

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;
            VkDescriptorSet m_vkDescriptorSet{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERVK_SRC_VULKAN_VULKANDESCRIPTORSET_H
