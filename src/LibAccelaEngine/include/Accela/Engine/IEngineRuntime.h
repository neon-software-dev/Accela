/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINERUNTIME_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINERUNTIME_H

#include <Accela/Engine/IKeyboardState.h>
#include <Accela/Engine/IMouseState.h>
#include <Accela/Engine/Scene/IWorldState.h>
#include <Accela/Engine/Scene/IWorldResources.h>

#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <memory>
#include <string>
#include <utility>

namespace Accela::Engine
{
    class Scene;

    /**
     * Main user-facing interface provided to Scenes which provides access to the engine
     */
    class IEngineRuntime
    {
        public:

            using Ptr = std::shared_ptr<IEngineRuntime>;

        public:

            virtual ~IEngineRuntime() = default;

            [[nodiscard]] virtual Common::ILogger::Ptr GetLogger() const noexcept = 0;
            [[nodiscard]] virtual Common::IMetrics::Ptr GetMetrics() const noexcept = 0;
            [[nodiscard]] virtual IWorldState::Ptr GetWorldState() const noexcept = 0;
            [[nodiscard]] virtual IWorldResources::Ptr GetWorldResources() const noexcept = 0;
            [[nodiscard]] virtual IKeyboardState::CPtr GetKeyboardState() const noexcept = 0;
            [[nodiscard]] virtual IMouseState::CPtr GetMouseState() const noexcept = 0;

            /**
             * @return The current simulation step tick index. Rolls over at uintmax_t ticks.
             */
            [[nodiscard]] virtual std::uintmax_t GetTickIndex() const noexcept = 0;

            /**
             * @return The current total time that's been simulated thus far, for a given scene, in
             * milliseconds. Rolls over at uintmax_t milliseconds.
             */
            [[nodiscard]] virtual std::uintmax_t GetSimulatedTime() const noexcept = 0;

            [[nodiscard]] virtual Render::RenderSettings GetRenderSettings() const noexcept = 0;
            virtual void SetRenderSettings(const Render::RenderSettings& settings) noexcept = 0;

            /**
             * Helper function which tells teh engine to keep the world audio listener's position synced
             * to where the world camera is currently located.
             *
             * @param sceneName The name of the scene to be manipulated
             * @param isSynced Whether or not to keep the audio listener synced to the world camera
             */
            virtual void SyncAudioListenerToWorldCamera(const std::string& sceneName, bool isSynced) = 0;

            /**
             * If set to true, physics collision bounds will be rendered. Note that this causes very
             * bad performance for complicated scenes and should only be used for debugging purposes.
             *
             * @param physicsDebugRender Whether or not to debug render the physics system.
             */
            virtual void SetPhysicsDebugRender(bool physicsDebugRender) = 0;

            /**
             * Instruct the engine to switch to a new scene. Will be performed after the current simulation
             * step. The current scene will be stopped and then the new scene started.
             *
             * @param scene The new scene to be used
             */
            virtual void SwitchToScene(std::unique_ptr<Scene> scene) = 0;

            /**
             * Instruct the engine to stop running. Will be performed after the current simulation step
             * has finished its work.
             */
            virtual void StopEngine() = 0;

            //
            // Desktop Only
            //

            /**
             * Whether or not to lock the cursor to the window's bounds
             */
            virtual void SetWindowCursorLock(bool lock) = 0;

            /**
             * Whether or not the engine window should be fullscreened
             */
            virtual void SetWindowFullscreen(bool fullscreen) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINERUNTIME_H
