/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_RENDERERBUILDER_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_RENDERERBUILDER_H

#include "IVulkanCalls.h"
#include "IVulkanContext.h"

#include <Accela/Render/IRenderer.h>
#include <Accela/Render/IOpenXR.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <string>

namespace Accela::Render
{
    /**
     * Builder class for building an IRenderer instance
     */
    class ACCELA_PUBLIC RendererBuilder
    {
        public:

            RendererBuilder(std::string appName,
                            uint32_t appVersion,
                            std::shared_ptr<IVulkanCalls> vulkanCalls,
                            std::shared_ptr<IVulkanContext> vulkanContext);

            RendererBuilder& WithLogger(Common::ILogger::Ptr logger);
            RendererBuilder& WithMetrics(Common::IMetrics::Ptr metrics);
            RendererBuilder& WithOpenXR(IOpenXR::Ptr openXR);

            [[nodiscard]] IRenderer::Ptr Build() const noexcept;

        private:

            std::string m_appName;
            uint32_t m_appVersion;
            std::shared_ptr<IVulkanCalls> m_vulkanCalls;
            std::shared_ptr<IVulkanContext> m_vulkanContext;

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IOpenXR::Ptr m_openXR;
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_RENDERERBUILDER_H
