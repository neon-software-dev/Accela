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

void Scene::OnSceneStart(const IEngineRuntime::Ptr& _engine)
{
    engine = _engine;
    m_events->OnSceneStart(engine);
}

void Scene::OnSceneStop()
{
    m_events->OnSceneStop();
}

void Scene::OnSimulationStep(unsigned int timeStep)
{
    m_events->OnSimulationStep(timeStep);
}

void Scene::OnKeyEvent(const Platform::KeyEvent& event)
{
    m_events->OnKeyEvent(event);
}

void Scene::OnMouseMoveEvent(const Platform::MouseMoveEvent& event)
{
    m_events->OnMouseMoveEvent(event);
}

void Scene::OnMouseButtonEvent(const Platform::MouseButtonEvent& event)
{
    m_events->OnMouseButtonEvent(event);
}

void Scene::OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event)
{
    m_events->OnPhysicsTriggerEvent(event);
}

}
