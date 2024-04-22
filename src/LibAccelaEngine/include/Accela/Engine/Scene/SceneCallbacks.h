/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENECALLBACKS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENECALLBACKS_H

#include <Accela/Engine/IEngineRuntime.h>

#include <Accela/Platform/Event/KeyEvent.h>
#include <Accela/Platform/Event/MouseMoveEvent.h>
#include <Accela/Platform/Event/MouseButtonEvent.h>

#include <memory>

namespace Accela::Engine
{
    /**
     * Callback events that a scene can generate. See: Scene class for documentation.
     */
    class SceneCallbacks
    {
        public:

            using Ptr = std::shared_ptr<SceneCallbacks>;

        public:

            virtual ~SceneCallbacks() = default;

            virtual void OnSceneStart(const IEngineRuntime::Ptr& engine) { (void)engine; };
            virtual void OnSceneStop(const IEngineRuntime::Ptr& engine) { (void)engine; };
            virtual void OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep) { (void)engine; (void)timeStep; };
            virtual void OnKeyEvent(const IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event) { (void)engine; (void)event; }
            virtual void OnMouseMoveEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event) { (void)engine; (void)event; }
            virtual void OnMouseButtonEvent(const IEngineRuntime::Ptr& engine, const Platform::MouseButtonEvent& event) { (void)engine; (void)event; }
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENECALLBACKS_H
