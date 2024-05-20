/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AccelaThread.h"

#include <Accela/Engine/Builder.h>
#include <Accela/Engine/Scene/WrappedScene.h>
#include <Accela/Engine/DesktopVulkanContext.h>

#include <Accela/Render/RendererBuilder.h>

#include <Accela/Platform/QtVulkanCalls.h>
#include <Accela/Platform/Window/QtWindow.h>

#include <Accela/Common/Log/StdLogger.h>
#include <Accela/Common/Metrics/InMemoryMetrics.h>

namespace Accela
{

AccelaThread::AccelaThread(QWindow *pWindow, EditorScene::Ptr scene)
    : m_pWindow(pWindow)
    , m_scene(std::move(scene))
    , m_logger(std::make_shared<Common::StdLogger>(Common::LogLevel::Warning))
    , m_metrics(std::make_shared<Common::InMemoryMetrics>())
    , m_platform(std::make_shared<Platform::PlatformQt>(m_logger))
{
    std::dynamic_pointer_cast<Platform::QtWindow>(m_platform->GetWindow())
        ->AttachToWindow(m_pWindow);
}

void AccelaThread::run()
{
    WaitForCommand();

    if (m_command == ThreadCommand::QuitEngine)
    {
        SignalEngineQuitFinished();
        return;
    }

    EngineRunLoop();
}

void AccelaThread::RunEngine()
{
    if (m_state == State::WaitingForCommand)
    {
        SignalThreadCommand(ThreadCommand::RunEngine);
        WaitForEngineInitFinished();
        return;
    }
}

void AccelaThread::QuitEngine()
{
    if (m_state == State::WaitingForCommand)
    {
        SignalThreadCommand(ThreadCommand::QuitEngine);
        return;
    }
    else if (m_state == State::RunningEngine)
    {
        m_scene->EnqueueCommand(std::make_shared<SceneQuitCommand>());
    }

    WaitForEngineQuitFinished();
}

void AccelaThread::WaitForCommand()
{
    std::unique_lock lk(m_commandMutex);
    m_commandCv.wait(lk, [this]{ return m_command != std::nullopt; });
}

void AccelaThread::WaitForEngineInitFinished()
{
    std::unique_lock lk(m_isInitFinishedMutex);
    m_isInitFinishedCv.wait(lk, [this]{ return isInitFinished; });
}

void AccelaThread::WaitForEngineQuitFinished()
{
    std::unique_lock lk(m_isQuitFinishedMutex);
    m_isQuitFinishedCv.wait(lk, [this]{ return isQuitFinished; });
}

void AccelaThread::SignalThreadCommand(AccelaThread::ThreadCommand command)
{
    std::lock_guard lk(m_commandMutex);
    m_command = command;
    m_commandCv.notify_all();
}

void AccelaThread::SignalEngineInitFinished()
{
    std::lock_guard lk(m_isInitFinishedMutex);
    isInitFinished = true;
    m_isInitFinishedCv.notify_all();
}

void AccelaThread::SignalEngineQuitFinished()
{
    std::lock_guard lk(m_isQuitFinishedMutex);
    isQuitFinished = true;
    m_isQuitFinishedCv.notify_all();
}

void AccelaThread::EngineRunLoop()
{
    if (!m_platform->Startup())
    {
        return;
    }

    const auto renderer = Render::RendererBuilder(
        "AccelaEditor",
        1,
        std::make_shared<Platform::QtVulkanCalls>(m_platform->GetQtVulkanInstance()),
        std::make_shared<Engine::DesktopVulkanContext>(m_platform)
    )
    .WithLogger(m_logger)
    .WithMetrics(m_metrics)
    .Build();

    //
    // Run
    //
    m_engine = Engine::Builder::Build(m_logger, m_metrics, m_platform, renderer);
    m_state = State::RunningEngine;

    m_engine->Run(
        std::make_unique<Engine::WrappedScene>(m_scene),
        false,
        [this](){ SignalEngineInitFinished(); }
    );

    //
    // Clean Up
    //
    m_scene = nullptr;
    m_engine = nullptr;
    m_platform->Shutdown();

    SignalEngineQuitFinished();
}

}
