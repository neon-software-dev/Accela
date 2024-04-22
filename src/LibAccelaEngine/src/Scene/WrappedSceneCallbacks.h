/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
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
            void OnSceneStop(const IEngineRuntime::Ptr& engine) override;
            void OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep) override;
            void OnKeyEvent(const IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event) override;
            void OnMouseMoveEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseButtonEvent& event) override;

        private:

            SceneCallbacks* m_pWrappedCallbacks;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_WRAPPEDSCENECALLBACKS_H
