/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 

#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANSURFACE
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANSURFACE

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>

namespace Accela::Render
{
    /**
     * Wrapper for working with surfaces
     */
    class VulkanSurface
    {
        public:

            VulkanSurface(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IVulkanContextPtr vulkanContext);

            /**
             * Create this surface
             *
             * @param instance The vulkan instance that will be used
             *
             * @return Whether the surface was created successfully
             */
            bool Create(const VulkanInstancePtr& instance) noexcept;

            /**
             * @return The VkSurfaceKHR object associated with this surface
             */
            [[nodiscard]] VkSurfaceKHR GetVkSurface() const noexcept { return m_vkSurface; }

            /**
             * @return The current pixel size of the surface
             */
            [[nodiscard]] std::pair<unsigned int, unsigned int> GetSurfaceSize() const;

            /**
             * Destroy this surface
             */
            void Destroy() noexcept;

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vulkanCalls;
            IVulkanContextPtr m_vulkanContext;

            VulkanInstancePtr m_instance;
            VkSurfaceKHR m_vkSurface{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANSURFACE
