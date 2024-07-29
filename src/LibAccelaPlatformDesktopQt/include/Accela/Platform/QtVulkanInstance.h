/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANINSTANCE_H
#define LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANINSTANCE_H

#include <Accela/Common/Log/ILogger.h>

#include <QVulkanInstance>

#include <vulkan/vulkan.h>

#include <memory>

namespace Accela::Platform
{
    /**
     * Wrapper around a QVulkanInstance that can be passed around and shared.
     *
     * When Init() is called, creates a new, default, QVulkanInstance which is
     * used by the engine for basic Vulkan queries in order to create its own
     * instance, at which point CreateFromVkInstance is called, which destroys
     * the default QVulkanInstance and then creates a new one based off of the
     * engine-provided VkInstance.
     */
    class QtVulkanInstance
    {
        public:

            using Ptr = std::shared_ptr<QtVulkanInstance>;

        public:

            explicit QtVulkanInstance(Common::ILogger::Ptr logger);

            [[nodiscard]] bool Init();
            void Destroy();

            [[nodiscard]] bool CreateFromVkInstance(VkInstance vkInstance);

            [[nodiscard]] QVulkanInstance* GetQVulkanInstance() const noexcept;

        private:

            Common::ILogger::Ptr m_logger;

            std::unique_ptr<QVulkanInstance> m_pVulkanInstance;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANINSTANCE_H
