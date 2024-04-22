/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_ENGINERUNTIME_H
#define LIBACCELAENGINE_SRC_ENGINERUNTIME_H

#include <Accela/Engine/IEngineRuntime.h>

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class IEngineAssets;
    struct RunState;

    class EngineRuntime : public IEngineRuntime
    {
        public:

            using Ptr = std::shared_ptr<EngineRuntime>;

        public:

            EngineRuntime(Common::ILogger::Ptr logger,
                          Common::IMetrics::Ptr metrics,
                          std::shared_ptr<IEngineAssets> assets,
                          std::shared_ptr<Render::IRenderer> renderer,
                          std::shared_ptr<RunState> runState);

            [[nodiscard]] Common::ILogger::Ptr GetLogger() const noexcept override;
            [[nodiscard]] Common::IMetrics::Ptr GetMetrics() const noexcept override;
            [[nodiscard]] IEngineAssets::Ptr GetAssets() const noexcept override;
            [[nodiscard]] IWorldState::Ptr GetWorldState() const noexcept override;
            [[nodiscard]] IWorldResources::Ptr GetWorldResources() const noexcept override;
            [[nodiscard]] IKeyboardState::CPtr GetKeyboardState() const noexcept override;

            [[nodiscard]] std::uintmax_t GetTickIndex() const noexcept override;
            [[nodiscard]] std::uintmax_t GetSimulatedTime() const noexcept override;

            [[nodiscard]] Render::RenderSettings GetRenderSettings() const noexcept override;
            void SetRenderSettings(const Render::RenderSettings& settings) noexcept override;
            [[nodiscard]] std::optional<Render::RenderSettings> ReceiveChangeRenderSettings();

            void SyncAudioListenerToWorldCamera(const std::string& sceneName, bool isSynced) override;
            [[nodiscard]] std::optional<std::string> GetSyncAudioListenerToWorldCamera() const;

            void SetPhysicsDebugRender(bool physicsDebugRender) override;
            [[nodiscard]] std::optional<bool> ReceiveSetPhysicsDebugRender();

            void SwitchToScene(std::unique_ptr<Scene> scene) override;
            [[nodiscard]] std::optional<std::shared_ptr<Scene>> ReceiveSceneSwitch();

            void StopEngine() override;
            [[nodiscard]] std::optional<bool> ReceiveStopEngine();

            void SetWindowCursorLock(bool lock) override;
            [[nodiscard]] std::optional<bool> ReceiveSetWindowCursorLock();

            void SetWindowFullscreen(bool fullscreen) override;
            [[nodiscard]] std::optional<bool> ReceiveSetWindowFullscreen();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Render::IRenderer> m_renderer;

            std::shared_ptr<RunState> m_runState;

            //
            // Persistent state the client can set
            //
            std::optional<std::string> m_syncAudioListenerToWorldCamera;

            //
            // Signals for the engine to process in its post simulation step
            //
            std::optional<Render::RenderSettings> m_changeRenderSettings;
            std::optional<std::shared_ptr<Scene>> m_sceneSwitch;
            std::optional<bool> m_stopEngine;
            std::optional<bool> m_windowCursorLock;
            std::optional<bool> m_windowFullscreen;
            std::optional<bool> m_physicsDebugRender;
    };
}

#endif //LIBACCELAENGINE_SRC_ENGINERUNTIME_H
