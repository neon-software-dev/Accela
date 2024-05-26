#include "PhysXPhysics.h"

#include "../Metrics.h"

#include <Accela/Common/BuildInfo.h>

#include <thread>
#include <array>
#include <algorithm>

namespace Accela::Engine
{

PhysXPhysics::PhysXPhysics(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           IWorldResourcesPtr worldResources)
   : m_logger(std::move(logger))
   , m_metrics(std::move(metrics))
   , m_worldResources(std::move(worldResources))
   , m_physXLogger(m_logger)
{
    InitPhysX();
}

PhysXPhysics::~PhysXPhysics()
{
    DestroyScenes();
    DestroyPhysX();
}

void PhysXPhysics::InitPhysX()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Initializing PhysX");

    m_pxFoundation = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        m_pxAllocator,
        m_physXLogger
    );

    // TODO Perf: What's the proper thread count value in relation to hardware threads?
    // TODO Perf: Evaluate perf of other "work wait mode" parameter values
    m_pxCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());

    m_pxPhysics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_pxFoundation,
        physx::PxTolerancesScale(),
        false, // TODO: Turn on for debug?
        nullptr
    );

    #ifdef ACCELA_USE_GPU_CUDA
        physx::PxCudaContextManagerDesc cudaContextManagerDesc{};
        m_pxCudaContextManager = PxCreateCudaContextManager(*m_pxFoundation, cudaContextManagerDesc, PxGetProfilerCallback());
    #endif
}

void PhysXPhysics::DestroyPhysX()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Destroying PhysX");

    PX_RELEASE(m_pxCudaContextManager)
    PX_RELEASE(m_pxPhysics)
    PX_RELEASE(m_pxCpuDispatcher)
    PX_RELEASE(m_pxFoundation)

    SyncMetrics();
}

void PhysXPhysics::DestroyScenes()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Destroying all Scenes");

    for (auto& scene : m_scenes)
    {
        scene.second.Destroy();
    }
    m_scenes.clear();

    // Note: We're purely destroying scenes here, but the mappings from
    // entity and player to scenes are left untouched

    SyncMetrics();
}

void PhysXPhysics::SimulationStep(unsigned int timeStep)
{
    // Get all scenes running their sim step in parallel
    for (auto& scene : m_scenes)
    {
        scene.second.StartSimulatingStep(timeStep);
    }

    // Wait for all scenes to finish simulating
    for (auto& scene : m_scenes)
    {
        scene.second.FinishSimulatingStep();
    }

    if (Common::BuildInfo::IsDebugBuild()) { DebugCheckResources(); }
}

std::optional<std::pair<RigidBody, bool>> PhysXPhysics::GetRigidBody(const EntityId& eid, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(eid, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::GetRigidBody: No such scene for eid: {}", eid);
        return std::nullopt;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::GetRigidBody: No such scene: {}", sceneName->name);
        return std::nullopt;
    }

    return sceneIt->second.GetRigidBody(eid);
}

void PhysXPhysics::MarkBodiesClean()
{
    for (auto& scene : m_scenes)
    {
        scene.second.MarkBodiesClean();
    }
}

std::unordered_map<PhysicsSceneName, std::queue<PhysicsTriggerEvent>> PhysXPhysics::PopTriggerEvents()
{
    std::unordered_map<PhysicsSceneName, std::queue<PhysicsTriggerEvent>> results;

    for (auto& scene : m_scenes)
    {
        results.insert({scene.first, scene.second.PopTriggerEvents()});
    }

    return results;
}

bool PhysXPhysics::CreateRigidBody(const PhysicsSceneName& scene, const EntityId& eid, const RigidBody& data)
{
    const auto sceneIt = m_scenes.find(scene);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::CreateRigidBody: No such scene: {}", scene.name);
        return false;
    }

    const bool result = sceneIt->second.CreateRigidBody(eid, data);

    SyncMetrics();

    return result;
}

bool PhysXPhysics::UpdateRigidBody(const EntityId& eid, const RigidBody& data, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(eid, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::UpdateRigidBody: Couldn't determine entity scene: {}", eid);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::UpdateRigidBody: No such scene: {}", sceneName->name);
        return false;
    }

    return sceneIt->second.UpdateRigidBody(eid, data);
}

bool PhysXPhysics::DestroyRigidBody(const EntityId& eid, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(eid, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::DestroyRigidBody: Couldn't determine entity scene: {}", eid);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::DestroyRigidBody: No such scene: {}", sceneName->name);
        return false;
    }

    const bool result = sceneIt->second.DestroyRigidBody(eid);

    SyncMetrics();

    return result;
}

bool PhysXPhysics::CreatePlayerController(const PhysicsSceneName& scene,
                                          const PlayerControllerName& player,
                                          const glm::vec3& position,
                                          const float& radius,
                                          const float& height,
                                          const PhysicsMaterial& material)
{
    const auto sceneName = GetSceneName(player, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::CreatePlayerController: Couldn't determine player scene: {}", player.name);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::CreatePlayerController: No such scene: {}", sceneName->name);
        return false;
    }

    const bool result = sceneIt->second.CreatePlayerController(player, position, radius, height, material);

    SyncMetrics();

    return result;
}

std::optional<glm::vec3> PhysXPhysics::GetPlayerControllerPosition(const PlayerControllerName& player, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(player, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::GetPlayerControllerPosition: Couldn't determine player scene: {}", player.name);
        return std::nullopt;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::GetPlayerControllerPosition: No such scene: {}", sceneName->name);
        return std::nullopt;
    }

    return sceneIt->second.GetPlayerControllerPosition(player);
}

std::optional<PlayerControllerState> PhysXPhysics::GetPlayerControllerState(const PlayerControllerName& player, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(player, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::PlayerControllerState: Couldn't determine player scene: {}", player.name);
        return std::nullopt;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::PlayerControllerState: No such scene: {}", sceneName->name);
        return std::nullopt;
    }

    return sceneIt->second.GetPlayerControllerState(player);
}

bool PhysXPhysics::SetPlayerControllerMovement(const PlayerControllerName& player,
                                               const glm::vec3& movement,
                                               const float& minDistance,
                                               const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(player, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::SetPlayerControllerMovement: Couldn't determine player scene: {}", player.name);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::SetPlayerControllerMovement: No such scene: {}", sceneName->name);
        return false;
    }

    return sceneIt->second.SetPlayerControllerMovement(player, movement, minDistance);
}

bool PhysXPhysics::SetPlayerControllerUpDirection(const PlayerControllerName& player,
                                                  const glm::vec3& upDirUnit,
                                                  const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(player, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::SetPlayerControllerUpDirection: Couldn't determine player scene: {}", player.name);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::SetPlayerControllerUpDirection: No such scene: {}", sceneName->name);
        return false;
    }

    return sceneIt->second.SetPlayerControllerUpDirection(player, upDirUnit);
}

bool PhysXPhysics::DestroyPlayerController(const PlayerControllerName& player, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(player, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::SetPlayerControllerUpDirection: Couldn't determine player scene: {}", player.name);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::SetPlayerControllerUpDirection: No such scene: {}", sceneName->name);
        return false;
    }

    const bool result = sceneIt->second.DestroyPlayerController(player);

    SyncMetrics();

    return result;
}

void PhysXPhysics::ClearAll()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Clearing All");

    for (auto& scene : m_scenes)
    {
        (void)scene.second.Clear();
    }

    SyncMetrics();
}

void PhysXPhysics::EnableDebugRenderOutput(bool enable)
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Enabling debug render output: {}", enable);

    for (auto& scene : m_scenes)
    {
        scene.second.EnableDebugRenderOutput(enable);
    }
}

std::vector<Render::Triangle> PhysXPhysics::GetDebugTriangles() const
{
    std::vector<Render::Triangle> results;

    for (auto& scene : m_scenes)
    {
        const auto sceneResults = scene.second.GetDebugTriangles();
        std::ranges::copy(sceneResults, std::back_inserter(results));
    }

    return results;
}

bool PhysXPhysics::CreateScene(const PhysicsSceneName& scene, const PhysicsSceneParams& params)
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Creating Scene: {}", scene.name);

    const auto it = m_scenes.find(scene);
    if (it != m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "PhysXPhysics::CreateScene: Scene already exists: {}", scene.name);
        return false;
    }

    PhysXScene physXScene(
        scene,
        params,
        m_logger,
        m_worldResources,
        m_pxPhysics,
        m_pxCpuDispatcher,
        m_pxCudaContextManager
    );
    if (!physXScene.Create())
    {
        m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Failed to create scene: {}", scene.name);
        return false;
    }

    m_scenes.insert({scene, physXScene});

    SyncMetrics();

    return true;
}

bool PhysXPhysics::DestroyScene(const PhysicsSceneName& scene)
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Destroying Scene: {}", scene.name);

    const auto it = m_scenes.find(scene);
    if (it == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "PhysXPhysics::DestroyScene: No such scene: {}", scene.name);
        return false;
    }

    it->second.Destroy();
    m_scenes.erase(scene);

    SyncMetrics();

    return true;
}

bool PhysXPhysics::ApplyLocalForceToRigidBody(const EntityId& eid, const glm::vec3& force, const std::optional<PhysicsSceneName>& scene)
{
    const auto sceneName = GetSceneName(eid, scene);
    if (!sceneName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::ApplyRigidBodyLocalForce: Couldn't determine entity scene: {}", eid);
        return false;
    }

    const auto sceneIt = m_scenes.find(*sceneName);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::ApplyRigidBodyLocalForce: No such scene: {}", sceneName->name);
        return false;
    }

    return sceneIt->second.ApplyLocalForceToRigidBody(eid, force);
}

std::vector<RaycastResult> PhysXPhysics::RaycastForCollisions(const PhysicsSceneName& scene,
                                                              const glm::vec3& rayStart_worldSpace,
                                                              const glm::vec3& rayEnd_worldSpace) const
{
    const auto sceneIt = m_scenes.find(scene);
    if (sceneIt == m_scenes.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::RaycastForCollisions: No such scene: {}", scene.name);
        return {};
    }

    return sceneIt->second.RaycastForCollisions(rayStart_worldSpace, rayEnd_worldSpace);
}

void PhysXPhysics::SyncMetrics()
{
    m_metrics->SetCounterValue(Engine_Physics_Scene_Count, m_scenes.size());

    std::size_t staticRigidBodiesCount = 0;
    std::size_t dynamicRigidBodiesCount = 0;

    for (const auto& scene : m_scenes)
    {
        staticRigidBodiesCount += scene.second.GetNumStaticRigidBodies();
        dynamicRigidBodiesCount += scene.second.GetNumDynamicRigidBodies();
    }

    m_metrics->SetCounterValue(Engine_Physics_Static_Rigid_Bodies_Count, staticRigidBodiesCount);
    m_metrics->SetCounterValue(Engine_Physics_Dynamic_Rigid_Bodies_Count, dynamicRigidBodiesCount);
}

void PhysXPhysics::DebugCheckResources()
{
    for (const auto& scene : m_scenes)
    {
        scene.second.DebugCheckResources();
    }
}

std::optional<PhysicsSceneName> PhysXPhysics::GetSceneName(const EntityId& eid, const std::optional<PhysicsSceneName>& scene) const
{
    if (scene) { return scene; }

    const auto it = m_entityToScene.find(eid);
    if (it == m_entityToScene.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

std::optional<PhysicsSceneName> PhysXPhysics::GetSceneName(const PlayerControllerName& player, const std::optional<PhysicsSceneName>& scene) const
{
    if (scene) { return scene; }

    const auto it = m_playerControllerToScene.find(player);
    if (it == m_playerControllerToScene.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
