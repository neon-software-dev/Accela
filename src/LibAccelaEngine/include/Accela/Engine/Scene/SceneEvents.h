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
            void OnSceneStop() override;
            void OnSimulationStep(unsigned int timeStep) override;
            void OnKeyEvent(const Platform::KeyEvent& event) override;
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override;
            void OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event) override;

        private:

            std::unordered_set<SceneCallbacks::Ptr> m_listeners;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENEEVENTS_H
