/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_ENGINE_H
#define LIBACCELAENGINE_SRC_ENGINE_H

#include "ForwardDeclares.h"
#include "KeyboardState.h"
#include "EngineRuntime.h"
#include "RunState.h"
#include "RenderTarget.h"

#include <Accela/Engine/IEngine.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <expected>
#include <memory>
#include <chrono>
#include <future>

namespace Accela::Platform
{
    class IPlatform;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class Engine : public IEngine
    {
        public:

            Engine(Common::ILogger::Ptr logger,
                   Common::IMetrics::Ptr metrics,
                   std::shared_ptr<Platform::IPlatform> platform,
                   std::shared_ptr<Render::IRenderer> renderer);

            void Run(Scene::UPtr initialScene, bool supportVRHeadset) override;

        private:

            bool InitializeRun(const RunState::Ptr& runState);
            void DestroyRun();

            void RunLoop(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void RunStep(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void SimulationStep(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void PostSimulationStep(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void ProcessEvents(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);

            void RenderFrame(const RunState::Ptr& runState);

            void ReceiveRenderSettingsChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void ReceiveSceneChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            static void SyncAudioListenerToWorldCamera(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void ReceiveEngineSettingsChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);
            void ReceivePhysicsDebugRenderChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            std::shared_ptr<Platform::IPlatform> m_platform;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Render::IRenderer> m_renderer;
            AudioManagerPtr m_audioManager;

            RenderTarget m_renderTarget;
    };
}

#endif //LIBACCELAENGINE_SRC_ENGINE_H
