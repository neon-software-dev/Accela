/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANSHADERMODULE
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANSHADERMODULE

#include "../ForwardDeclares.h"

#include <Accela/Render/Shader/ShaderSpec.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <spirv_reflect.h>

#include <memory>
#include <string>
#include <optional>

namespace Accela::Render
{
    /**
     * Wrapper for working with vulkan shader modules
     */
    class VulkanShaderModule
    {
        public:

            VulkanShaderModule(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, VulkanDevicePtr device);

            /**
             * Create this shader module
             *
             * @param shaderSpec The specification this module should use
             *
             * @return Whether the module was created successfully
             */
            bool Create(const ShaderSpec& shaderSpec);

            [[nodiscard]] std::optional<ShaderSpec> GetShaderSpec() const noexcept { return m_shaderSpec; }
            [[nodiscard]] SpvReflectShaderModule GetReflectInfo() const noexcept { return m_reflectInfo; }
            [[nodiscard]] VkShaderModule GetVkShaderModule() const noexcept { return m_vkShaderModule; }

            void Destroy();

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;

            std::optional<ShaderSpec> m_shaderSpec;
            SpvReflectShaderModule m_reflectInfo{};
            VkShaderModule m_vkShaderModule{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANSHADERMODULE
