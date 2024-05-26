#ifndef LIBACCELAENGINE_SRC_SCENE_WRAPPEDSCENECALLBACKS_H
#define LIBACCELAENGINE_SRC_SCENE_WRAPPEDSCENECALLBACKS_H

#include <Accela/Engine/Scene/SceneCallbacks.h>

namespace Accela::Engine
{
    /**
     * Wraps a raw SceneCallbacks pointer
     */
    class WrappedSceneCallbacks : public SceneCallbacks
    {
        public:

            explicit WrappedSceneCallbacks(SceneCallbacks* pWrappedCalls);

            void OnSceneStart(const IEngineRuntime::Ptr& engine) override;
            void OnSceneStop() override;
            void OnSimulationStep(unsigned int timeStep) override;
            void OnKeyEvent(const Platform::KeyEvent& event) override;
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override;

        private:

            SceneCallbacks* m_pWrappedCallbacks;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_WRAPPEDSCENECALLBACKS_H
