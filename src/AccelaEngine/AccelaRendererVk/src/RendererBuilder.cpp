/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Render/RendererBuilder.h>
#include <Accela/Render/StubOpenXR.h>

#include "RendererVk.h"

#include <Accela/Common/Log/StubLogger.h>
#include <Accela/Common/Metrics/StubMetrics.h>

namespace Accela::Render
{

RendererBuilder::RendererBuilder(std::string appName,
                                 uint32_t appVersion,
                                 std::shared_ptr<IVulkanCalls> vulkanCalls,
                                 std::shared_ptr<IVulkanContext> vulkanContext)
    : m_appName(std::move(appName))
    , m_appVersion(appVersion)
    , m_vulkanCalls(std::move(vulkanCalls))
    , m_vulkanContext(std::move(vulkanContext))
    , m_logger(std::make_shared<Common::StubLogger>())
    , m_metrics(std::make_shared<Common::StubMetrics>())
    , m_openXR(std::make_shared<StubOpenXR>())
{

}

RendererBuilder& RendererBuilder::WithLogger(Common::ILogger::Ptr logger)
{
    m_logger = std::move(logger);
    return *this;
}

RendererBuilder& RendererBuilder::WithMetrics(Common::IMetrics::Ptr metrics)
{
    m_metrics = std::move(metrics);
    return *this;
}

RendererBuilder& RendererBuilder::WithOpenXR(IOpenXR::Ptr openXR)
{
    m_openXR = std::move(openXR);
    return *this;
}

IRenderer::Ptr RendererBuilder::Build() const noexcept
{
    return std::make_shared<RendererVk>(m_appName, m_appVersion, m_logger, m_metrics, m_vulkanCalls, m_vulkanContext, m_openXR);
}

}
