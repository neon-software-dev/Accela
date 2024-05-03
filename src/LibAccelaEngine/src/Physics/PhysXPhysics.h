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

#include "../ForwardDeclares.h"

#include <Accela/Engine/Physics/IPhysicsRuntime.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <unordered_map>

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
            void PostSimulationSyncRigidBodyEntity(const EntityId& eid,
                                                   PhysicsComponent& physicsComponent,
                                                   TransformComponent& transformComponent) override;
            void CreateRigidBodyFromEntity(const EntityId& eid,
                                           const PhysicsComponent& physicsComponent,
                                           const TransformComponent& transformComponent,
                                           const BoundsComponent& boundsComponent) override;
            void UpdateRigidBodyFromEntity(const EntityId& eid,
                                           const PhysicsComponent& physicsComponent,
                                           const TransformComponent& transformComponent) override;
            void DestroyRigidBody(const EntityId& eid) override;

            [[nodiscard]] bool CreatePlayerController(const std::string& name,
                                                      const glm::vec3& position,
                                                      const float& radius,
                                                      const float& height) override;
            [[nodiscard]] std::optional<glm::vec3> GetPlayerControllerPosition(const std::string& name) override;
            [[nodiscard]] std::optional<PlayerControllerState> GetPlayerControllerState(const std::string& name) override;
            [[nodiscard]] bool SetPlayerControllerMovement(const std::string& name,
                                                           const glm::vec3& movement,
                                                           const float& minDistance) override;
            void DestroyPlayerController(const std::string& name) override;

            void ClearAll() override;
            void EnableDebugRenderOutput(bool enable) override;
            [[nodiscard]] std::vector<Render::Triangle> GetDebugTriangles() const override;

            //
            // IPhysicsRuntime
            //
            bool ApplyRigidBodyLocalForce(const EntityId& eid, const glm::vec3& force) override;

            [[nodiscard]] std::vector<RaycastResult> RaycastForCollisions(
                const glm::vec3& rayStart_worldSpace,
                const glm::vec3& rayEnd_worldSpace) const override;

        private:

            struct PhysXRigidActor
            {
                PhysXRigidActor() = default;

                PhysXRigidActor(PhysicsBodyType _bodyType, physx::PxRigidActor* _pRigidActor)
                    : bodyType(_bodyType)
                    , pRigidActor(_pRigidActor)
                { }

                PhysicsBodyType bodyType{PhysicsBodyType::Static};
                physx::PxRigidActor* pRigidActor{nullptr};
            };

            struct PhysxMovement
            {
                PhysxMovement(glm::vec3 _movement, float _minDistance)
                    : movement(_movement)
                    , minDistance(_minDistance)
                { }

                glm::vec3 movement;
                float minDistance;
            };

            struct PhysXPlayerController
            {
                explicit PhysXPlayerController(physx::PxController* _pPxController)
                    : pPxController(_pPxController)
                { }

                std::optional<PhysxMovement> movementCommand;
                physx::PxController* pPxController{nullptr};
                std::size_t msSinceLastUpdate{0};
            };

        private:

            void InitPhysX();
            void DestroyPhysX();

            void CreateScene();
            void DestroyScene();

            void ApplyPlayerControllerMovements(unsigned int timeStep);

            [[nodiscard]] PhysXRigidActor CreatePhysXRigidActor(const PhysicsComponent& physicsComponent);

            static void SyncRigidActorData(PhysXRigidActor& physxActor,
                                           const PhysicsComponent& physicsComponent,
                                           const TransformComponent& transformComponent);

            [[nodiscard]] bool CreateRigidActorShape(PhysXRigidActor& physxActor,
                                                     const TransformComponent& transformComponent,
                                                     const BoundsComponent& boundsComponent);

            [[nodiscard]] physx::PxShape* CreateRigidActorShape_AABB(const BoundsComponent& boundsComponent,
                                                                     const TransformComponent& transformComponent,
                                                                     glm::vec3& localPositionAdjustment);

            [[nodiscard]] physx::PxShape* CreateRigidActorShape_Capsule(const BoundsComponent& boundsComponent,
                                                                        const TransformComponent& transformComponent,
                                                                        glm::vec3& localPositionAdjustment);

            [[nodiscard]] physx::PxShape* CreateRigidActorShape_Sphere(const BoundsComponent& boundsComponent,
                                                                       const TransformComponent& transformComponent,
                                                                       glm::vec3& localPositionAdjustment);

            [[nodiscard]] physx::PxShape* CreateRigidActorShape_HeightMap(const BoundsComponent& boundsComponent,
                                                                          const TransformComponent& transformComponent,
                                                                          glm::vec3& localPositionAdjustment,
                                                                          glm::quat& localOrientationAdjustment);

            [[nodiscard]] static inline physx::PxRigidDynamic* GetRigidDynamic(const PhysXRigidActor& rigidActor);

            void SyncMetrics();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResourcesPtr m_worldResources;

            // PhysX global
            PhysxLogger m_physXLogger;
            physx::PxDefaultAllocator m_pxAllocator;
            physx::PxFoundation* m_pxFoundation{nullptr};
            physx::PxDefaultCpuDispatcher* m_pxCpuDispatcher{nullptr};
            physx::PxPhysics* m_pxPhysics{nullptr};

            // Scene specific
            physx::PxScene* m_pxScene{nullptr};
            physx::PxControllerManager* m_pxControllerManager{nullptr};
            physx::PxMaterial* m_pxDefaultMaterial{nullptr};

            std::unordered_map<EntityId, PhysXRigidActor> m_entityToRigidActor;
            std::unordered_map<std::string, PhysXPlayerController> m_playerControllers;
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSXPHYSICS_H
