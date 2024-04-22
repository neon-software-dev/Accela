/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/EngineDesktop.h>
#include <Accela/Engine/Builder.h>
#include <Accela/Engine/IEngine.h>
#include <Accela/Platform/PlatformSDL.h>
#include <Accela/Platform/SDLWindow.h>
#include <Accela/Render/RendererBuilder.h>

#include "DesktopVulkanCalls.h"
#include "DesktopVulkanContext.h"

namespace Accela::Engine
{

EngineDesktop::EngineDesktop(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
{

}

bool EngineDesktop::Startup()
{
    m_logger->Log(Common::LogLevel::Info, "EngineDesktop: Starting");

    auto sdlPlatform = std::make_shared<Platform::PlatformSDL>(m_logger);
    if (!sdlPlatform->Startup())
    {
        m_logger->Log(Common::LogLevel::Fatal, "EngineDesktop: Failed to start SDL platform");
        Shutdown();
        return false;
    }
    m_platform = sdlPlatform;

    return true;
}

void EngineDesktop::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "EngineDesktop: Shutting down");

    if (m_platform != nullptr)
    {
        std::static_pointer_cast<Platform::PlatformSDL>(m_platform)->Shutdown();
        m_platform = nullptr;
    }
}

void EngineDesktop::Run(const std::string& appName,
                        const uint32_t& appVersion,
                        const WindowParams& windowParams,
                        VROutput vrOutput,
                        Scene::UPtr initialScene)
{
    m_logger->Log(Common::LogLevel::Info, "EngineDesktop: Run starting");

    //
    // Create a desktop window for display
    //
    const auto sdlPlatform = std::static_pointer_cast<Platform::PlatformSDL>(m_platform);
    const auto sdlWindow = std::dynamic_pointer_cast<Platform::SDLWindow>(m_platform->GetWindow());

    const auto pWindow = sdlWindow->CreateWindow(
        windowParams.windowTitle,
        windowParams.windowSize.w,
        windowParams.windowSize.h
    );
    if (pWindow == nullptr)
    {
        return;
    }

    //
    // Create a renderer for the engine to use
    //
    const auto renderer = Render::RendererBuilder(
        appName,
        appVersion,
        std::make_shared<DesktopVulkanCalls>(),
        std::make_shared<DesktopVulkanContext>(sdlPlatform)
    )
    .WithLogger(m_logger)
    .WithMetrics(m_metrics)
    .Build();

    //
    // Create the engine and give control to it
    //
    auto engine = Builder::Build(m_logger, m_metrics, m_platform, renderer);

    const bool supportVRHeadset = vrOutput == VROutput::Supported;

    engine->Run(std::move(initialScene), supportVRHeadset);

    //
    // Cleanup after the engine has finished running
    //
    m_logger->Log(Common::LogLevel::Info, "EngineDesktop: Run stopping");
}

}
