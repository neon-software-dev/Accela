/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENECALLBACKS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENECALLBACKS_H

#include <Accela/Engine/IEngineRuntime.h>
#include <Accela/Engine/Physics/PhysicsCommon.h>

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
            virtual void OnSceneStop() { };
            virtual void OnSimulationStep(unsigned int timeStep) { (void)timeStep; };
            virtual void OnKeyEvent(const Platform::KeyEvent& event) { (void)event; }
            virtual void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) { (void)event; }
            virtual void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) { (void)event; }
            virtual void OnPhysicsTriggerEvent(const PhysicsTriggerEvent& event) { (void)event; }
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_SCENECALLBACKS_H
