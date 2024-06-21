/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANPHYSICALDEVICE
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANPHYSICALDEVICE

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace Accela::Render
{
    /**
     * Wrapper for working with a physical device
     */
    class VulkanPhysicalDevice
    {
        public:

            VulkanPhysicalDevice(Common::ILogger::Ptr logger,
                                 IVulkanCallsPtr vulkanCalls,
                                 IVulkanContextPtr vulkanContext,
                                 VkPhysicalDevice vkPhysicalDevice);

            /**
             * @return A vector of all physical devices detected
             */
            static std::vector<VulkanPhysicalDevicePtr> EnumerateAll(const Common::ILogger::Ptr& logger,
                                                                     const IVulkanCallsPtr& vulkanCalls,
                                                                     const IVulkanContextPtr& vulkanContext,
                                                                     const VulkanInstancePtr& instance);

            /**
             * Determines whether this physical device is suitable for use in the engine. It must support the
             * required queue family types, be of the appropriate GPU type, and be able to support presenting
             * to our surface.
             *
             * @param surface The surface that the physical device would need to interact with
             *
             * @return Whether the physical devive is suitable for use in the engine
             */
            [[nodiscard]] bool IsDeviceSuitable(const VulkanSurfacePtr& surface) const;

            /**
             * @return A suitability rating to compare this physical device to other physical devices installed
             * on the current system.
             */
            [[nodiscard]] uint32_t GetDeviceRating() const;

            /**
             * @return The VkPhysicalDevice object associated with this physical device.
             */
            [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const { return m_vkPhysicalDevice; }

            /**
             * @return The vendor-defined name of this physical device.
             */
            [[nodiscard]] std::string GetDeviceName() const;

            [[nodiscard]] const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const noexcept {
                return m_vkPhysicalDeviceProperties;
            }

            [[nodiscard]] const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatures() const noexcept {
                return m_vkPhysicalDeviceFeatures;
            }

            /**
             * @return The queue family index that supports graphics commands, or std::nullopt if none exists.
             */
            [[nodiscard]] std::optional<uint32_t> GetGraphicsQueueFamilyIndex() const;

            /**
             * @return The queue family index that supports compute commands, or std::nullopt if none exists.
             */
            [[nodiscard]] std::optional<uint32_t> GetComputeQueueFamilyIndex() const;

            /**
             * @param surface Surface to be tested against
             *
             * @return The queue family index that supports presenting to the specified surface, or std::nullopt
             * if none exists.
             */
            [[nodiscard]] std::optional<uint32_t> GetPresentQueueFamilyIndex(const VulkanSurfacePtr& surface) const;

            [[nodiscard]] static VkFormat GetDepthBufferFormat() noexcept { return VK_FORMAT_D32_SFLOAT; }

            [[nodiscard]] VkSampleCountFlagBits GetMaxUsableSampleCount() const;

        private:

            [[nodiscard]] bool SupportsExtension(const std::string& extensionName) const;

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vulkanCalls;
            IVulkanContextPtr m_vulkanContext;
            VkPhysicalDevice m_vkPhysicalDevice;

            VkPhysicalDeviceProperties m_vkPhysicalDeviceProperties{};
            VkPhysicalDeviceFeatures m_vkPhysicalDeviceFeatures{};
            std::vector<VkQueueFamilyProperties> m_vkQueueFamilyProperties;
            std::vector<VkExtensionProperties> m_vkExtensionProperties;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANPHYSICALDEVICE
