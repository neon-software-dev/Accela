/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENEEVENTS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENEEVENTS_H

#include "SceneCallbacks.h"

#include <unordered_set>
#include <memory>

namespace Accela::Engine
{
    /**
     * Provided by a scene, listeners can be registered to receive scene callbacks
     */
    class SceneEvents : public SceneCallbacks
    {
        public:

            using Ptr = std::shared_ptr<SceneEvents>;

        public:

            SceneEvents() = default;
            ~SceneEvents() override;

            SceneEvents(const SceneEvents&) = delete;
            SceneEvents& operator=(const SceneEvents&) = delete;

            void RegisterListener(const SceneCallbacks::Ptr& sceneCalls);
            void DeregisterListener(const SceneCallbacks::Ptr& sceneCalls);
            void DeregisterAll();

            //
            // SceneCallbacks
            //
            void OnSceneStart(const IEngineRuntime::Ptr& engine) override;
            void OnSceneStop(const IEngineRuntime::Ptr& engine) override;
            void OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep) override;
            void OnKeyEvent(const IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event) override;
            void OnMouseMoveEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseButtonEvent& event) override;

        private:

            std::unordered_set<SceneCallbacks::Ptr> m_listeners;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENEEVENTS_H
