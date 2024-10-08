/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANINSTANCE_H
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANINSTANCE_H

#include "VulkanCommon.h"

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace Accela::Render
{
    struct DisableValidationErrorLogging
    {
        DisableValidationErrorLogging();
        ~DisableValidationErrorLogging();
    };

    /**
     * Wrapper class for VkInstance functionality
     */
    class VulkanInstance
    {
        public:

            VulkanInstance(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IVulkanContextPtr vulkanContext);

            /**
             * Create a new vulkan instance
             *
             * @param appName The name of the client app
             * @param appVersion The version of the client app
             * @param enableValidationLayers Whether validation layers should be enabled, if available
             * @param extraRequiredInstanceExtensions Extra instance extensions that are required and should be enabled
             * @param requiredMinVulkanVersion The minimum version of Vulkan that's required
             * @param desiredMaxVulkanVersion Optional max version of Vulkan that's desired (not required)
             *
             * @return Whether the instance was created successfully
             */
            bool CreateInstance(const std::string& appName,
                                uint32_t appVersion,
                                bool enableValidationLayers,
                                const std::vector<std::string>& extraRequiredInstanceExtensions,
                                uint32_t requiredMinVulkanVersion = VULKAN_API_VERSION,
                                const std::optional<uint32_t>& desiredMaxVulkanVersion = std::nullopt);

            /**
             * @return The VkInstance object associated with this instance
             */
            [[nodiscard]] VkInstance GetVkInstance() const noexcept { return m_vkInstance; }

            /**
             * Configures whether debug/validation messages should be output to the logger
             */
            static void SetShouldLogDebugMessages(bool shouldLog);

            /**
             * Destroy this vulkan instance
             */
            void Destroy() noexcept;

        private:

            struct LayerProperties
            {
                std::string layerName;
                std::vector<VkExtensionProperties> extensions; // Layer-provided extensions
            };

            struct InstanceProperties
            {
                std::vector<VkExtensionProperties> extensions; // Instance-provided extensions
                std::vector<LayerProperties> layers; // Available layers
            };

        private:

            [[nodiscard]] bool VerifyVulkanVersion(uint32_t requiredMinVulkanVersion,
                                                   const std::optional<uint32_t>& desiredMaxVulkanVersion) const;

            [[nodiscard]] InstanceProperties GatherInstanceProperties() const;
            static bool InstanceSupportsExtension(const InstanceProperties& properties, const std::string& extensionName);
            static bool InstanceSupportsLayer(const InstanceProperties& properties, const std::string& layerName);
            static std::optional<std::string> FindLayerSupportingExtension(const InstanceProperties& properties, const std::string& extensionName);

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vulkanCalls;
            IVulkanContextPtr m_vulkanContext;

            VkInstance m_vkInstance{VK_NULL_HANDLE};
            VkDebugUtilsMessengerEXT m_vkDebugMessenger{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANINSTANCE_H
