/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include <Accela/Engine/Scene/SceneEvents.h>

namespace Accela::Engine
{

SceneEvents::~SceneEvents()
{
    DeregisterAll();
}

void SceneEvents::RegisterListener(const SceneCallbacks::Ptr& sceneCalls)
{
    m_listeners.insert(sceneCalls);
}

void SceneEvents::DeregisterListener(const SceneCallbacks::Ptr& sceneCalls)
{
    m_listeners.erase(sceneCalls);
}

void SceneEvents::DeregisterAll()
{
    m_listeners.clear();
}

void SceneEvents::OnSceneStart(const IEngineRuntime::Ptr& engine)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnSceneStart(engine);
    }
}

void SceneEvents::OnSceneStop(const IEngineRuntime::Ptr& engine)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnSceneStop(engine);
    }
}

void SceneEvents::OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnSimulationStep(engine, timeStep);
    }
}

void SceneEvents::OnKeyEvent(const IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnKeyEvent(engine, event);
    }
}

void SceneEvents::OnMouseMoveEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnMouseMoveEvent(engine, event);
    }
}

void SceneEvents::OnMouseButtonEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseButtonEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnMouseButtonEvent(engine, event);
    }
}

}
