/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_WRAPPEDSCENE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_WRAPPEDSCENE_H

#include "Scene.h"

namespace Accela::Engine
{
    class WrappedScene : public Scene
    {
        public:

            explicit WrappedScene(Scene::Ptr scene) : m_scene(std::move(scene)) {}

            ~WrappedScene() override = default;

            [[nodiscard]] std::string GetName() const override { return m_scene->GetName(); }
            [[nodiscard]] SceneEvents::Ptr GetEvents() const noexcept override { return m_scene->GetEvents(); }
            void OnSceneStart(const IEngineRuntime::Ptr& _engine) override { return m_scene->OnSceneStart(_engine); }
            void OnSceneStop() override { return m_scene->OnSceneStop(); }
            void OnSimulationStep(unsigned int timeStep) override { return m_scene->OnSimulationStep(timeStep); }
            void OnKeyEvent(const Platform::KeyEvent& event) override { return m_scene->OnKeyEvent(event); }
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override { return m_scene->OnMouseMoveEvent(event); }
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override { return m_scene->OnMouseButtonEvent(event); }
            void OnMouseWheelEvent(const Platform::MouseWheelEvent& event) override { return m_scene->OnMouseWheelEvent(event); }
            void OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event) override { return m_scene->OnPhysicsTriggerEvent(event); }

        private:

            Scene::Ptr m_scene;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_WRAPPEDSCENE_H
