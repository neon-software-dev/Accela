/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
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

void SceneEvents::OnSceneStop()
{
    for (const auto& listener : m_listeners)
    {
        listener->OnSceneStop();
    }
}

void SceneEvents::OnSimulationStep(unsigned int timeStep)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnSimulationStep(timeStep);
    }
}

void SceneEvents::OnKeyEvent(const Platform::KeyEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnKeyEvent(event);
    }
}

void SceneEvents::OnMouseMoveEvent(const Platform::MouseMoveEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnMouseMoveEvent(event);
    }
}

void SceneEvents::OnMouseButtonEvent(const Platform::MouseButtonEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnMouseButtonEvent(event);
    }
}

void SceneEvents::OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event)
{
    for (const auto& listener : m_listeners)
    {
        listener->OnPhysicsTriggerEvent(event);
    }
}

}
