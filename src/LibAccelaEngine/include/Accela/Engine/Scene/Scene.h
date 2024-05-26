#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENE_H

#include "SceneCallbacks.h"
#include "SceneEvents.h"

#include <memory>
#include <string>

namespace Accela::Engine
{
    /**
     * Main user-facing subclass to derive from to define a scene that the engine can run
     */
    class Scene : public SceneCallbacks
    {
        public:

            using UPtr = std::unique_ptr<Scene>;
            using Ptr = std::shared_ptr<Scene>;

        public:

            Scene();
            ~Scene() override = default;

            /**
             * @return A unique name to identify this scene. Mostly only used for debugging purposes.
             */
            [[nodiscard]] virtual std::string GetName() const = 0;

            /**
             * @return Access to the SceneEvents system where listeners to be registered to observe scene events.
             */
            [[nodiscard]] virtual SceneEvents::Ptr GetEvents() const noexcept;

            /** Called when the scene is first started, before any other callbacks, and never again */
            void OnSceneStart(const IEngineRuntime::Ptr& engine) override;

            /** Called when the scene is being stopped, and no other callbacks afterwards */
            void OnSceneStop() override;

            /** Called every time the engine runs another simulation step */
            void OnSimulationStep(unsigned int timeStep) override;

            /** Called when a keypress event occurs */
            void OnKeyEvent(const Platform::KeyEvent& event) override;

            /** Called when a mouse movement event occurs */
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override;

            /** Called when a mouse button event occurs */
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override;

            /** Called when a physics trigger has been triggered */
            void OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event) override;

        protected:

            IEngineRuntime::Ptr engine;

        private:

            SceneEvents::Ptr m_events;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENE_H
