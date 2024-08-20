/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_WRAPPEDSCENE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_WRAPPEDSCENE_H

#include "Scene.h"

#include <Accela/Common/SharedLib.h>

namespace Accela::Engine
{
    class ACCELA_PUBLIC WrappedScene : public Scene
    {
        public:

            explicit WrappedScene(Scene::Ptr scene);

            ~WrappedScene() override = default;

            [[nodiscard]] std::string GetName() const override;
            [[nodiscard]] SceneEvents::Ptr GetEvents() const noexcept override;
            void OnSceneStart(const IEngineRuntime::Ptr& _engine) override;
            void OnSceneStop() override;
            void OnSimulationStep(unsigned int timeStep) override;
            void OnKeyEvent(const Platform::KeyEvent& event) override;
            void OnTextInputEvent(const Platform::TextInputEvent& event) override;
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override;
            void OnMouseWheelEvent(const Platform::MouseWheelEvent& event) override;
            void OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event) override;

        private:

            Scene::Ptr m_scene;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_WRAPPEDSCENE_H
