/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_PHYSXPHYSICS_H
#define LIBACCELAENGINE_SRC_PHYSICS_PHYSXPHYSICS_H

#include "IPhysics.h"
#include "PhysXWrapper.h"
#include "PhysXLogger.h"
#include "PhysXScene.h"

#include "../ForwardDeclares.h"

#include <Accela/Engine/Physics/IPhysicsRuntime.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>

namespace Accela::Engine
{
    class PhysXPhysics : public IPhysics, public IPhysicsRuntime
    {
        public:

            PhysXPhysics(Common::ILogger::Ptr logger,
                         Common::IMetrics::Ptr metrics,
                         IWorldResourcesPtr worldResources);
            ~PhysXPhysics() override;

            //
            // IPhysics
            //
            void SimulationStep(unsigned int timeStep) override;
            [[nodiscard]] std::optional<std::pair<RigidBody, bool>> GetRigidBody(const EntityId& eid,
                                                                                 const std::optional<PhysicsSceneName>& scene) override;
            void MarkBodiesClean() override;
            [[nodiscard]] std::unordered_map<PhysicsSceneName, std::queue<PhysicsTriggerEvent>> PopTriggerEvents() override;

            [[nodiscard]] bool CreateRigidBody(const PhysicsSceneName& scene, const EntityId& eid, const RigidBody& rigidBody) override;
            [[nodiscard]] bool UpdateRigidBody(const EntityId& eid, const RigidBody& rigidBody, const std::optional<PhysicsSceneName>& scene) override;
            [[nodiscard]] bool DestroyRigidBody(const EntityId& eid, const std::optional<PhysicsSceneName>& scene) override;

            [[nodiscard]] bool CreatePlayerController(const PhysicsSceneName& player,
                                                      const PlayerControllerName& name,
                                                      const glm::vec3& position,
                                                      const float& radius,
                                                      const float& height,
                                                      const PhysicsMaterial& material) override;
            [[nodiscard]] std::optional<glm::vec3> GetPlayerControllerPosition(const PlayerControllerName& player,
                                                                               const std::optional<PhysicsSceneName>& scene) override;
            [[nodiscard]] std::optional<PlayerControllerState> GetPlayerControllerState(const PlayerControllerName& player,
                                                                                        const std::optional<PhysicsSceneName>& scene) override;
            [[nodiscard]] bool SetPlayerControllerMovement(const PlayerControllerName& player,
                                                           const glm::vec3& movement,
                                                           const float& minDistance,
                                                           const std::optional<PhysicsSceneName>& scene) override;
            [[nodiscard]] bool SetPlayerControllerUpDirection(const PlayerControllerName& player,
                                                              const glm::vec3& upDirUnit,
                                                              const std::optional<PhysicsSceneName>& scene) override;
            [[nodiscard]] bool DestroyPlayerController(const PlayerControllerName& name, const std::optional<PhysicsSceneName>& scene) override;

            void ClearAll() override;

            void EnableDebugRenderOutput(bool enable) override;
            [[nodiscard]] std::vector<Render::Triangle> GetDebugTriangles() const override;

            //
            // IPhysicsRuntime
            //
            [[nodiscard]] bool CreateScene(const PhysicsSceneName& scene, const PhysicsSceneParams& params) override;
            [[nodiscard]] bool DestroyScene(const PhysicsSceneName& scene) override;

            bool ApplyLocalForceToRigidBody(const EntityId& eid, const glm::vec3& force, const std::optional<PhysicsSceneName>& scene) override;

            [[nodiscard]] std::vector<RaycastResult> RaycastForCollisions(
                const PhysicsSceneName& scene,
                const glm::vec3& rayStart_worldSpace,
                const glm::vec3& rayEnd_worldSpace) const override;

        private:

            void InitPhysX();
            void DestroyPhysX();

            void DestroyScenes();

            [[nodiscard]] std::optional<PhysicsSceneName> GetSceneName(const EntityId& eid, const std::optional<PhysicsSceneName>& scene) const;
            [[nodiscard]] std::optional<PhysicsSceneName> GetSceneName(const PlayerControllerName& player, const std::optional<PhysicsSceneName>& scene) const;

            void SyncMetrics();
            void DebugCheckResources();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResourcesPtr m_worldResources;

            // PhysX Global
            PhysxLogger m_physXLogger;
            physx::PxDefaultAllocator m_pxAllocator;
            physx::PxFoundation* m_pxFoundation{nullptr};
            physx::PxDefaultCpuDispatcher* m_pxCpuDispatcher{nullptr};
            physx::PxPhysics* m_pxPhysics{nullptr};
            physx::PxCudaContextManager* m_pxCudaContextManager{nullptr};

            // PhysX Scenes
            std::unordered_map<PhysicsSceneName, PhysXScene> m_scenes;
            std::unordered_map<EntityId, PhysicsSceneName> m_entityToScene;
            std::unordered_map<PlayerControllerName, PhysicsSceneName> m_playerControllerToScene;
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSXPHYSICS_H
