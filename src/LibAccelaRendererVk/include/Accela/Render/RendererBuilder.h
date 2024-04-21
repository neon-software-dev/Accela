/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_RENDERERBUILDER_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_RENDERERBUILDER_H

#include "IVulkanCalls.h"
#include "IVulkanContext.h"

#include <Accela/Render/IRenderer.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <string>

namespace Accela::Render
{
    /**
     * Builder class for building an IRenderer instance
     */
    class RendererBuilder
    {
        public:

            RendererBuilder(std::string appName,
                            uint32_t appVersion,
                            std::shared_ptr<IVulkanCalls> vulkanCalls,
                            std::shared_ptr<IVulkanContext> vulkanContext);

            RendererBuilder& WithLogger(Common::ILogger::Ptr logger);
            RendererBuilder& WithMetrics(Common::IMetrics::Ptr metrics);

            [[nodiscard]] IRenderer::Ptr Build() const noexcept;

        private:

            std::string m_appName;
            uint32_t m_appVersion;
            std::shared_ptr<IVulkanCalls> m_vulkanCalls;
            std::shared_ptr<IVulkanContext> m_vulkanContext;

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_RENDERERBUILDER_H
