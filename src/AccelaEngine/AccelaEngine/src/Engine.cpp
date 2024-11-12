/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Engine.h"
#include "ShaderUtil.h"
#include "Metrics.h"
#include "EngineRuntime.h"
#include "RunState.h"

#include "Scene/WorldState.h"
#include "Scene/WorldLogic.h"
#include "Scene/WorldResources.h"
#include "Scene/TextureResources.h"
#include "Scene/PackageResources.h"
#include "Audio/AudioManager.h"
#include "Media/MediaManager.h"
#include "Physics/PhysXPhysics.h"
#include "Component/RenderableStateComponent.h"

#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Platform/IPlatform.h>
#include <Accela/Render/IRenderer.h>
#include <Accela/Render/Graph/RenderGraphNodes.h>

#include <Accela/Common/Timer.h>

namespace Accela::Engine
{

Engine::Engine(Common::ILogger::Ptr logger,
               Common::IMetrics::Ptr metrics,
               std::shared_ptr<Platform::IPlatform> platform,
               std::shared_ptr<Render::IRenderer> renderer)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_platform(std::move(platform))
    , m_renderer(std::move(renderer))
{

}

void Engine::Run(Scene::UPtr initialScene, Render::OutputMode renderOutputMode, const std::function<void()>& onInitCallback)
{
    m_logger->Log(Common::LogLevel::Info, "AccelaEngine: Run start");

    const auto renderResolution = Render::USize(1920, 1080);
    const auto virtualResolution = glm::vec2(1920, 1080);

    Render::RenderSettings renderSettings{};
    renderSettings.presentMode = Render::PresentMode::Immediate;
    renderSettings.presentScaling = Render::PresentScaling::CenterInside;
    renderSettings.resolution = renderResolution;

    const auto audioManager = std::make_shared<AudioManager>(m_logger);
    const auto worldResources = std::make_shared<WorldResources>(m_logger, m_renderer, m_platform->GetFiles(), m_platform->GetText(), audioManager);
    const auto physics = std::make_shared<PhysXPhysics>(m_logger, m_metrics, worldResources);
    const auto mediaManager = std::make_shared<MediaManager>(m_logger, m_metrics, worldResources, audioManager, m_renderer);
    const auto worldState = std::make_shared<WorldState>(m_logger, m_metrics, worldResources, m_platform->GetWindow(), m_renderer, audioManager, mediaManager, physics, renderSettings, virtualResolution);

    const auto runState = std::make_shared<RunState>(std::move(initialScene), worldResources, worldState, m_platform, audioManager, mediaManager);
    const auto runtime = std::make_shared<EngineRuntime>(m_logger, m_metrics, m_renderer, runState);

    if (!InitializeRun(runState, renderOutputMode))
    {
        m_logger->Log(Common::LogLevel::Fatal, "AccelaEngine: Failed to initialize the run");
        return;
    }

    std::invoke(onInitCallback);

    RunLoop(runtime, runState);

    DestroyRun(runState);

    m_logger->Log(Common::LogLevel::Info, "AccelaEngine: Run finish");
}

bool Engine::InitializeRun(const RunState::Ptr& runState, Render::OutputMode renderOutputMode)
{
    m_logger->Log(Common::LogLevel::Info, "AccelaEngine: Initializing the engine run");

    const auto worldState = std::dynamic_pointer_cast<WorldState>(runState->worldState);

    //
    // Start the renderer
    //
    const auto assetsShadersExpect = ReadShadersFromAssets(m_logger, m_platform->GetFiles());
    if (!assetsShadersExpect)
    {
        m_logger->Log(Common::LogLevel::Fatal, "AccelaEngine: Failed to load shaders from assets");
        return false;
    }

    Render::RenderInit renderInit{};
    renderInit.outputMode = renderOutputMode;
    renderInit.shaders = *assetsShadersExpect;

    if (!m_renderer->Startup(renderInit, worldState->GetRenderSettings()))
    {
        m_logger->Log(Common::LogLevel::Fatal, "AccelaEngine: Failed to initialize the renderer");
        return false;
    }

    //
    // Configure a render target for the scene to be rendered into
    //
    m_renderTargetId = m_renderer->GetIds()->renderTargetIds.GetId();

    if (!m_renderer->CreateRenderTarget(m_renderTargetId, "Offscreen").get())
    {
        m_logger->Log(Common::LogLevel::Fatal, "AccelaEngine: Failed to create render target");
        m_renderer->GetIds()->renderTargetIds.ReturnId(m_renderTargetId);
        m_renderTargetId = {};
        return false;
    }

    //
    // Start the audio manager
    //
    if (!runState->audioManager->Startup())
    {
        m_logger->Log(Common::LogLevel::Fatal, "AccelaEngine: Failed to start the audio manager");
        return false;
    }

    //
    // Start the media manager
    //
    if (!runState->mediaManager->Startup())
    {
        m_logger->Log(Common::LogLevel::Fatal, "AccelaEngine: Failed to start the media manager");
        return false;
    }

    return true;
}

void Engine::DestroyRun(const RunState::Ptr& runState)
{
    m_logger->Log(Common::LogLevel::Info, "AccelaEngine: Destroying the engine run");

    runState->mediaManager->Shutdown();
    runState->audioManager->Shutdown();
    m_renderer->Shutdown();

    m_renderTargetId = {};
}

void Engine::RunLoop(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    m_logger->Log(Common::LogLevel::Info, "Engine: Starting initial scene: {}", runState->scene->GetName());
    runState->scene->OnSceneStart(runtime);

    while (runState->keepRunning && !runtime->ReceiveStopEngine())
    {
        RunStep(runtime, runState);
    }

    m_logger->Log(Common::LogLevel::Info, "Engine: Stopping scene: {}", runState->scene->GetName());
    runState->scene->OnSceneStop();

    m_logger->Log(Common::LogLevel::Info, "Engine: Cleaning up resources");
    runtime->GetWorldResources()->DestroyAll();
}

void Engine::RunStep(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    //
    // If the previous frame's render request has finished then queue up another to render
    // the current state of the scene. Wait up to a timestep amount of time for an in-progress
    // render to finish. If it still hasn't finished then continue on and run the update logic
    // below to consume that accumulated time.
    //
    if (!runState->previousFrameRenderedFuture.valid() ||
        runState->previousFrameRenderedFuture.wait_for(std::chrono::milliseconds(runState->timeStep)) == std::future_status::ready)
    {
        RenderFrame(runState);
    }

    //
    // Advance the simulation in fixed time steps to sync up to how much real time has passed
    //
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> producedTime = currentTime - runState->lastTimeSync;
    runState->lastTimeSync = currentTime;

    // If we're unable to advance the engine in real time we need to cap the number of steps we're
    // taking in any given loop, or else we'll enter a death spiral. Just disconnect the simulation
    // from real time and simulate slowly until the load lessens.
    if (producedTime.count() >= runState->maxProducedTimePerLoop)
    {
        m_logger->Log(Common::LogLevel::Warning, "Simulation falling behind!");
        producedTime = std::chrono::duration<double, std::milli>(runState->maxProducedTimePerLoop);
    }

    runState->accumulatedTime += producedTime.count();

    //
    // Consume accumulated time by advancing the simulation forward in discrete steps
    //
    while (runState->accumulatedTime >= runState->timeStep)
    {
        // Perform a simulation step
        SimulationStep(runtime, runState);
        runState->accumulatedTime -= runState->timeStep;
    }
}

void Engine::SimulationStep(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    Common::Timer simulationStepTimer(Engine_SimulationStep_Time);

    //
    // Process any OS events that have happened since the last simulation step
    //
    ProcessEvents(runState);

    //
    // Tell the scene to run a step
    //
    {
        Common::Timer sceneSimulationStepTimer(Engine_SceneSimulationStep_Time);
        runState->scene->OnSimulationStep(runState->timeStep);
        sceneSimulationStepTimer.StopTimer(m_metrics);
    }

    //
    // Do any post simulation step tasks, including running internal engine systems
    // that sync to / process changes that the scene made.
    //
    PostSimulationStep(runtime, runState);

    simulationStepTimer.StopTimer(m_metrics);
}

void Engine::PostSimulationStep(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    const auto worldState = std::static_pointer_cast<WorldState>(runState->worldState);

    //
    // Respond to any changes the scene requested
    //

    // Process setting update requests
    ReceiveEngineSettingsChange(runtime, runState);

    // If the scene told us to change render settings, do so now
    ReceiveRenderSettingsChange(runtime, runState);

    // If the scene asked us to switch to a new scene, do so now
    ReceiveSceneChange(runtime, runState);

    // If the scene asked us to set physics debug rendering, do so now
    ReceivePhysicsDebugRenderChange(runtime, runState);

    //
    // Update World State
    //

    // Keep the audio listener's position synced to the world camera, if requested
    SyncAudioListenerToWorldCamera(runtime, runState);

    // Execute ECS systems
    worldState->ExecuteSystems(runState);

    //
    // Update our tick index now that a simulation step has finished
    //
    runState->tickIndex++;
}

void Engine::ReceiveRenderSettingsChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    const auto renderSettingsOpt = runtime->ReceiveChangeRenderSettings();
    if (!renderSettingsOpt) { return; }

    m_logger->Log(Common::LogLevel::Info, "Engine: Performing render settings change");

    // Tell the renderer to change its render settings
    m_renderer->ChangeRenderSettings(*renderSettingsOpt);

    // As the virtual -> render space sprite transform depends on the render resolution, we need to invalidate
    // all sprite renderables when render settings change. RendererSyncSystem will update all sprite renderables
    // in the renderer with new data.
    std::dynamic_pointer_cast<WorldState>(runState->worldState)->MarkSpritesDirty();
}

void Engine::ReceiveSceneChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    const auto sceneSwitchOpt = runtime->ReceiveSceneSwitch();
    if (!sceneSwitchOpt) { return; }

    m_logger->Log(Common::LogLevel::Info, "Engine: Performing scene switch");

    //
    // Clean up from the old scene
    //

    m_logger->Log(Common::LogLevel::Info, "Engine: Stopping scene: {}", runState->scene->GetName());

    // Stop and destroy the old scene
    runState->scene->OnSceneStop();
    runState->scene = nullptr;

    // Clear out physics system state that the previous scene had created
    std::dynamic_pointer_cast<IPhysics>(runState->worldState->GetPhysics())->ClearAll();

    //
    // Set up the new scene
    //

    runState->scene = *sceneSwitchOpt;

    m_logger->Log(Common::LogLevel::Info, "Engine: Starting scene: {}", runState->scene->GetName());

    // Start the new scene
    runState->scene->OnSceneStart(runtime);
}

void Engine::SyncAudioListenerToWorldCamera(const EngineRuntime::Ptr& runtime, const RunState::Ptr& runState)
{
    const auto sceneNameOpt = runtime->GetSyncAudioListenerToWorldCamera();
    if (!sceneNameOpt) { return; }

    const auto worldState = std::static_pointer_cast<WorldState>(runState->worldState);

    worldState->SyncAudioListenerToCamera(worldState->GetOrCreateSceneState(*sceneNameOpt).worldCamera);
}

void Engine::ReceiveEngineSettingsChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr&)
{
    // Event to lock the cursor to the window
    const auto windowCursorLockEvent = runtime->ReceiveSetWindowCursorLock();
    if (windowCursorLockEvent)
    {
        if (!m_platform->GetWindow()->LockCursorToWindow(*windowCursorLockEvent))
        {
            m_logger->Log(Common::LogLevel::Error,
              "Engine::ReceiveEngineSettingsChange: Failed to apply cursor lock setting");
        }
    }

    // Event to fullscreen the window
    const auto windowFullscreenEvent = runtime->ReceiveSetWindowFullscreen();
    if (windowFullscreenEvent)
    {
        if (!m_platform->GetWindow()->SetFullscreen(*windowFullscreenEvent))
        {
            m_logger->Log(Common::LogLevel::Error,
              "Engine::ReceiveEngineSettingsChange: Failed to apply fullscreen setting");
        }
    }
}

void Engine::ReceivePhysicsDebugRenderChange(const EngineRuntime::Ptr& runtime, const RunState::Ptr&)
{
    const auto physicsDebugRenderEvent = runtime->ReceiveSetPhysicsDebugRender();
    if (physicsDebugRenderEvent)
    {
        std::dynamic_pointer_cast<IPhysics>(runtime->GetWorldState()->GetPhysics())
            ->EnableDebugRenderOutput(*physicsDebugRenderEvent);
    }
}

void Engine::ProcessEvents(const RunState::Ptr& runState)
{
    const auto worldState = std::dynamic_pointer_cast<WorldState>(runState->worldState);
    const auto renderSettings = worldState->GetRenderSettings();

    std::queue<Platform::SystemEvent> events = m_platform->GetEvents()->PopLocalEvents();

    while (!events.empty())
    {
        Platform::SystemEvent event = events.front();
        events.pop();

        if (std::holds_alternative<Platform::KeyEvent>(event))
        {
            const auto keyEvent = std::get<Platform::KeyEvent>(event);
            runState->scene->OnKeyEvent(keyEvent);
        }
        else if (std::holds_alternative<Platform::TextInputEvent>(event))
        {
            const auto textInputEvent = std::get<Platform::TextInputEvent>(event);
            runState->scene->OnTextInputEvent(textInputEvent);
        }
        else if (std::holds_alternative<Platform::MouseMoveEvent>(event))
        {
            auto mouseMoveEvent = std::get<Platform::MouseMoveEvent>(event);

            // Convert the clicked point from window space to render space
            const auto renderPointOpt = WindowPointToRenderPoint(
                renderSettings,
                Render::Size(m_platform->GetWindow()->GetWindowSize().value()),
                {mouseMoveEvent.xPos, mouseMoveEvent.yPos}
            );

            // Do nothing if the mouse was moved within the window but outside the draw/render area
            if (!renderPointOpt) { return; }

            // Convert the point from render space to virtual space
            const auto virtualPoint = RenderPointToVirtualPoint(renderSettings, runState->worldState->GetVirtualResolution(), *renderPointOpt);

            // Rewrite the mouse move event's coordinates to be relative to virtual space
            mouseMoveEvent.xPos = virtualPoint.x;
            mouseMoveEvent.yPos = virtualPoint.y;

            runState->scene->OnMouseMoveEvent(mouseMoveEvent);
        }
        else if (std::holds_alternative<Platform::MouseButtonEvent>(event))
        {
            auto mouseButtonEvent = std::get<Platform::MouseButtonEvent>(event);

            // Convert the clicked point from window space to render space
            const auto renderPointOpt = WindowPointToRenderPoint(
                renderSettings,
                Render::Size(m_platform->GetWindow()->GetWindowSize().value()),
                {mouseButtonEvent.xPos, mouseButtonEvent.yPos}
            );

            // Do nothing if the mouse was clicked within the window but outside the draw/render area
            if (!renderPointOpt) { return; }

            // Convert the point from render space to virtual space
            const auto virtualPoint = RenderPointToVirtualPoint(renderSettings, runState->worldState->GetVirtualResolution(), *renderPointOpt);

            // Rewrite the mouse button event's coordinates to be relative to virtual space
            mouseButtonEvent.xPos = (unsigned int)virtualPoint.x;
            mouseButtonEvent.yPos = (unsigned int)virtualPoint.y;

            runState->scene->OnMouseButtonEvent(mouseButtonEvent);
        }
        else if (std::holds_alternative<Platform::MouseWheelEvent>(event))
        {
            auto mouseWheelEvent = std::get<Platform::MouseWheelEvent>(event);

            runState->scene->OnMouseWheelEvent(mouseWheelEvent);
        }
        else if (std::holds_alternative<Platform::WindowResizeEvent>(event))
        {
            m_renderer->SurfaceChanged();
        }
        else if (std::holds_alternative<Platform::WindowCloseEvent>(event))
        {
            m_logger->Log(Common::LogLevel::Info, "ProcessEvents: Detected window close event, stopping engine");
            runState->keepRunning = false;
        }
    }
}

void Engine::RenderFrame(const RunState::Ptr& runState)
{
    using namespace Render;

    const std::string scene = DEFAULT_SCENE;  // TODO: Make configurable

    const auto worldState = std::dynamic_pointer_cast<WorldState>(runState->worldState);
    const auto& sceneState = worldState->GetOrCreateSceneState(scene);
    const auto physics = std::dynamic_pointer_cast<IPhysics>(worldState->GetPhysics());

    const glm::vec2 virtualRes = runState->worldState->GetVirtualResolution();
    const glm::vec2 renderRes{worldState->GetRenderSettings().resolution.w, worldState->GetRenderSettings().resolution.h};

    // Conversion ratio from virtual resolution space to render resolution space
    const glm::vec3 virtualRatio{virtualRes.x / renderRes.x, virtualRes.y / renderRes.y, 1.0f};

    // Offset the sprite camera by half the virtual resolution so that the middle of the virtual resolution
    // corresponds to no camera translation
    const glm::vec3 spriteCameraOffset = glm::vec3(virtualRes / 2.0f, 0.0f);

    RenderCamera worldRenderCamera{};
    worldRenderCamera.position = sceneState.worldCamera->GetPosition();
    worldRenderCamera.lookUnit = sceneState.worldCamera->GetLookUnit();
    worldRenderCamera.upUnit = sceneState.worldCamera->GetUpUnit();
    worldRenderCamera.rightUnit = sceneState.worldCamera->GetRightUnit();
    worldRenderCamera.fovYDegrees = sceneState.worldCamera->GetFovYDegrees();
    worldRenderCamera.aspectRatio = renderRes.x / renderRes.y;

    RenderCamera spriteRenderCamera{};
    spriteRenderCamera.position = (sceneState.spriteCamera->GetPosition() - spriteCameraOffset) / virtualRatio;
    spriteRenderCamera.lookUnit = sceneState.spriteCamera->GetLookUnit();
    spriteRenderCamera.rightUnit = sceneState.spriteCamera->GetRightUnit();
    spriteRenderCamera.upUnit = sceneState.spriteCamera->GetUpUnit();

    RenderParams renderParams;
    renderParams.worldRenderCamera = worldRenderCamera;
    renderParams.spriteRenderCamera = spriteRenderCamera;
    renderParams.ambientLightIntensity = sceneState.ambientLightIntensity;
    renderParams.ambientLightColor = sceneState.ambientLightColor;
    renderParams.skyBoxTextureId = sceneState.skyBoxTextureId;
    renderParams.skyBoxViewTransform = sceneState.skyBoxViewTransform;
    renderParams.highlightedObjects = GetHighlightedObjects(runState);
    renderParams.debugTriangles = physics->GetDebugTriangles();

    PresentConfig presentConfig{};
    presentConfig.clearColor = {worldState->GetRenderSettings().presentClearColor, 1.0f};

    RenderGraph::Ptr renderGraph = std::make_unique<RenderGraph>();

    renderGraph
        ->StartWith<RenderGraphNode_RenderScene>(scene, m_renderTargetId, renderParams)
        ->AndThen<RenderGraphNode_Present>(m_renderTargetId, presentConfig);

    runState->previousFrameRenderedFuture = m_renderer->RenderFrame(renderGraph);
}

std::unordered_set<Render::ObjectId> Engine::GetHighlightedObjects(const RunState::Ptr& runState)
{
    const auto worldState = std::dynamic_pointer_cast<WorldState>(runState->worldState);

    const auto highlightedEntities = worldState->GetHighlightedEntities();

    std::unordered_set<Render::ObjectId> highlightedObjects;
    highlightedObjects.reserve(highlightedEntities.size());

    for (const auto& entity : highlightedEntities)
    {
        const auto stateComponent = worldState->GetComponent<RenderableStateComponent>(entity);
        if (stateComponent)
        {
            if (stateComponent->type != RenderableStateComponent::Type::Object &&
                stateComponent->type != RenderableStateComponent::Type::Model)
            {
                continue;
            }

            for (const auto& renderableId : stateComponent->renderableIds)
            {
                highlightedObjects.insert(Render::ObjectId(renderableId.second.id));
            }
        }
    }

    return highlightedObjects;
}

}
