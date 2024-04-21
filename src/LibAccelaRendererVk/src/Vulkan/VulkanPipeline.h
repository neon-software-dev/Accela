/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANPIPELINE
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANPIPELINE

#include "../ForwardDeclares.h"

#include "../Pipeline/PipelineConfig.h"

#include <Accela/Common/Log/ILogger.h>

#include <vector>

namespace Accela::Render
{
    /**
     * Wrapper for working with a vulkan pipeline
     */
    class VulkanPipeline
    {
        public:

            VulkanPipeline(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, IShadersPtr shaders, VulkanDevicePtr device);

            /**
             * Create this vulkan pipeline
             *
             * @param config The pipeline configuration to use
             *
             * @return Whether the pipeline was created successfully
             */
            bool Create(const PipelineConfig& config);

            /**
             * Destroys this pipeline
             */
            void Destroy();

            [[nodiscard]] std::size_t GetConfigHash() const noexcept { return m_config.GetUniqueKey(); }

            /**
             * @return The VkPipeline object associated with this pipeline
             */
            [[nodiscard]] VkPipeline GetVkPipeline() const noexcept { return m_vkPipeline; }

            /**
             * @return The VkPipelineLayout object describing this pipeline
             */
            [[nodiscard]] VkPipelineLayout GetVkPipelineLayout() const noexcept { return m_vkPipelineLayout; }

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            IShadersPtr m_shaders;
            VulkanDevicePtr m_device;

            PipelineConfig m_config{};
            VkPipelineLayout m_vkPipelineLayout{VK_NULL_HANDLE};
            VkPipeline m_vkPipeline{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANPIPELINE
