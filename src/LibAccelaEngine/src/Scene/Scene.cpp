/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include <Accela/Engine/Scene/Scene.h>

namespace Accela::Engine
{

Scene::Scene()
    : m_events(std::make_shared<SceneEvents>())
{

}

SceneEvents::Ptr Scene::GetEvents() const noexcept
{
    return m_events;
}

void Scene::OnSceneStart(const IEngineRuntime::Ptr& engine)
{
    m_events->OnSceneStart(engine);
}

void Scene::OnSceneStop(const IEngineRuntime::Ptr& engine)
{
    m_events->OnSceneStop(engine);
}

void Scene::OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep)
{
    m_events->OnSimulationStep(engine, timeStep);
}

void Scene::OnKeyEvent(const IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event)
{
    m_events->OnKeyEvent(engine, event);
}

void Scene::OnMouseMoveEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event)
{
    m_events->OnMouseMoveEvent(engine, event);
}

void Scene::OnMouseButtonEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseButtonEvent& event)
{
    m_events->OnMouseButtonEvent(engine, event);
}

}
