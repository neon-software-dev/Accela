/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "EngineRuntime.h"
#include "RunState.h"
#include "Scene/WorldState.h"

#include <Accela/Engine/IEngineAssets.h>
#include <Accela/Engine/Scene/Scene.h>

#include <Accela/Render/IRenderer.h>

namespace Accela::Engine
{

EngineRuntime::EngineRuntime(Common::ILogger::Ptr logger,
                             Common::IMetrics::Ptr metrics,
                             std::shared_ptr<IEngineAssets> assets,
                             std::shared_ptr<Render::IRenderer> renderer,
                             std::shared_ptr<RunState> runState)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_assets(std::move(assets))
    , m_renderer(std::move(renderer))
    , m_runState(std::move(runState))
{

}

Common::ILogger::Ptr EngineRuntime::GetLogger() const noexcept { return m_logger; }
Common::IMetrics::Ptr EngineRuntime::GetMetrics() const noexcept { return m_metrics; }
IEngineAssets::Ptr EngineRuntime::GetAssets() const noexcept { return m_assets; }
IWorldState::Ptr EngineRuntime::GetWorldState() const noexcept { return m_runState->worldState; }
IWorldResources::Ptr EngineRuntime::GetWorldResources() const noexcept { return m_runState->worldResources; }
IKeyboardState::CPtr EngineRuntime::GetKeyboardState() const noexcept { return m_runState->keyboardState; }
std::uintmax_t EngineRuntime::GetTickIndex() const noexcept { return m_runState->tickIndex; }
std::uintmax_t EngineRuntime::GetSimulatedTime() const noexcept { return m_runState->tickIndex * m_runState->timeStep; }
Render::RenderSettings EngineRuntime::GetRenderSettings() const noexcept { return std::dynamic_pointer_cast<WorldState>(m_runState->worldState)->GetRenderSettings(); }

template <typename T>
T ReceiveSignal(T& v)
{
    auto cpy = v;
    v = std::nullopt;
    return cpy;
}

void EngineRuntime::SetRenderSettings(const Render::RenderSettings& settings) noexcept
{
    std::dynamic_pointer_cast<WorldState>(m_runState->worldState)->SetRenderSettings(settings);

    m_changeRenderSettings = settings;
}

std::optional<Render::RenderSettings> EngineRuntime::ReceiveChangeRenderSettings()
{
    return ReceiveSignal(m_changeRenderSettings);
}

void EngineRuntime::SyncAudioListenerToWorldCamera(const std::string& sceneName, bool isSynced)
{
    if (isSynced)
    {
        m_syncAudioListenerToWorldCamera = sceneName;
    }
    else
    {
        m_syncAudioListenerToWorldCamera = std::nullopt;
    }
}

std::optional<std::string> EngineRuntime::GetSyncAudioListenerToWorldCamera() const
{
    return m_syncAudioListenerToWorldCamera;
}

void EngineRuntime::SetPhysicsDebugRender(bool physicsDebugRender)
{
    m_physicsDebugRender = physicsDebugRender;
}

std::optional<bool> EngineRuntime::ReceiveSetPhysicsDebugRender()
{
    return ReceiveSignal(m_physicsDebugRender);
}


void EngineRuntime::SwitchToScene(std::unique_ptr<Scene> scene)
{
    m_sceneSwitch = std::move(scene);
}

std::optional<std::shared_ptr<Scene>> EngineRuntime::ReceiveSceneSwitch()
{
    return ReceiveSignal(m_sceneSwitch);
}

void EngineRuntime::StopEngine()
{
    m_stopEngine = true;
}

std::optional<bool> EngineRuntime::ReceiveStopEngine()
{
    return ReceiveSignal(m_stopEngine);
}

void EngineRuntime::SetWindowCursorLock(bool lock)
{
    m_windowCursorLock = lock;
}

std::optional<bool> EngineRuntime::ReceiveSetWindowCursorLock()
{
    return ReceiveSignal(m_windowCursorLock);
}

void EngineRuntime::SetWindowFullscreen(bool fullscreen)
{
    m_windowFullscreen = fullscreen;
}

std::optional<bool> EngineRuntime::ReceiveSetWindowFullscreen()
{
    return ReceiveSignal(m_windowFullscreen);
}

}
