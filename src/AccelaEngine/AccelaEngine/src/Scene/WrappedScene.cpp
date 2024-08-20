/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Scene/WrappedScene.h>

namespace Accela::Engine
{

WrappedScene::WrappedScene(Scene::Ptr scene)
    : m_scene(std::move(scene))
{

}

std::string WrappedScene::GetName() const { return m_scene->GetName(); }
SceneEvents::Ptr WrappedScene::GetEvents() const noexcept { return m_scene->GetEvents(); }
void WrappedScene::OnSceneStart(const IEngineRuntime::Ptr& _engine) { return m_scene->OnSceneStart(_engine); }
void WrappedScene::OnSceneStop() { return m_scene->OnSceneStop(); }
void WrappedScene::OnSimulationStep(unsigned int timeStep) { return m_scene->OnSimulationStep(timeStep); }
void WrappedScene::OnKeyEvent(const Platform::KeyEvent& event) { return m_scene->OnKeyEvent(event); }
void WrappedScene::OnTextInputEvent(const Platform::TextInputEvent& event) { return m_scene->OnTextInputEvent(event); }
void WrappedScene::OnMouseMoveEvent(const Platform::MouseMoveEvent& event) { return m_scene->OnMouseMoveEvent(event); }
void WrappedScene::OnMouseButtonEvent(const Platform::MouseButtonEvent& event) { return m_scene->OnMouseButtonEvent(event); }
void WrappedScene::OnMouseWheelEvent(const Platform::MouseWheelEvent& event) { return m_scene->OnMouseWheelEvent(event); }
void WrappedScene::OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event) { return m_scene->OnPhysicsTriggerEvent(event); }

}
