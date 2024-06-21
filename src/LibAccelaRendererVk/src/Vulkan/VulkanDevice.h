/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANDEVICE
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANDEVICE

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    /**
     * Wrapper for working with a Vulkan device
     */
    class VulkanDevice
    {
        public:

            VulkanDevice(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IVulkanContextPtr vulkanContext);

            /**
             * Create a new logical device and queues for submitting work to it.
             *
             * @param physicalDevice The physical device to use
             * @param presentQueueFamilyIndex The queue family index to use for presenting
             *
             * @return Whether the device was created successfully
             */
            bool Create(const VulkanPhysicalDevicePtr& physicalDevice, const VulkanSurfacePtr& surface);

            /**
             * @return The VkDevice object associated with this device
             */
            [[nodiscard]] VkDevice GetVkDevice() const noexcept{ return m_vkDevice; }

            /**
             * @return A graphics-capable queue
             */
            [[nodiscard]] VkQueue GetVkGraphicsQueue() const noexcept { return m_vkGraphicsQueue; }

            /**
             * @return A presentation-capable queue
             */
            [[nodiscard]] VkQueue GetVkPresentQueue() const noexcept { return m_vkPresentQueue; }

            /**
            * @return A compute-capable queue. (May or may not be the same queue as GetVkGraphicsQueue())
            */
            [[nodiscard]] VkQueue GetVkComputeQueue() const noexcept { return m_vkComputeQueue; }

            /**
             * Destroy the device + queues
             */
            void Destroy() noexcept;

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vulkanCalls;
            IVulkanContextPtr m_vulkanContext;

            VkDevice m_vkDevice{VK_NULL_HANDLE};
            VkQueue m_vkGraphicsQueue{VK_NULL_HANDLE};
            VkQueue m_vkPresentQueue{VK_NULL_HANDLE};
            VkQueue m_vkComputeQueue{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANDEVICE
