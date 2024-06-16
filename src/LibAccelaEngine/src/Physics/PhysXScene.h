/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_PHYSXSCENE_H
#define LIBACCELAENGINE_SRC_PHYSICS_PHYSXSCENE_H

#include "PhysXWrapper.h"
#include "RigidBody.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Physics/IPhysicsRuntime.h>

#include <Accela/Render/Util/Triangle.h>

#include <Accela/Common/Log/ILogger.h>

#include <unordered_map>
#include <string>
#include <queue>

namespace Accela::Engine
{
    class IWorldResources;

    class PhysXScene : private physx::PxSimulationEventCallback
    {
        public:

            PhysXScene(PhysicsSceneName name,
                       PhysicsSceneParams params,
                       Common::ILogger::Ptr logger,
                       std::shared_ptr<IWorldResources> worldResources,
                       physx::PxPhysics* pPhysics,
                       physx::PxCpuDispatcher* pCpuDispatcher,
                       physx::PxCudaContextManager* pCudaContextManager);

            [[nodiscard]] bool Create();
            [[nodiscard]] bool Clear();
            void Destroy();

            void StartSimulatingStep(unsigned int timeStep);
            void FinishSimulatingStep();

            void MarkBodiesClean();

            [[nodiscard]] std::queue<PhysicsTriggerEvent> PopTriggerEvents();

            [[nodiscard]] bool ApplyLocalForceToRigidBody(const EntityId& eid, const glm::vec3& force) const;
            void EnableDebugRenderOutput(bool enable);
            [[nodiscard]] std::vector<Render::Triangle> GetDebugTriangles() const;

            //
            // Rigid Bodies
            //
            [[nodiscard]] bool CreateRigidBody(const EntityId& eid, const RigidBody& rigidBody);
            [[nodiscard]] std::optional<std::pair<RigidBody, bool>> GetRigidBody(const EntityId& eid);
            [[nodiscard]] bool UpdateRigidBody(const EntityId& eid, const RigidBody& rigidBody);
            [[nodiscard]] bool DestroyRigidBody(const EntityId& eid);

            //
            // Player Controllers
            //
            [[nodiscard]] bool CreatePlayerController(const PlayerControllerName& player,
                                                      const glm::vec3& position,
                                                      const float& radius,
                                                      const float& height,
                                                      const PhysicsMaterial& material);
            [[nodiscard]] std::optional<glm::vec3> GetPlayerControllerPosition(const PlayerControllerName& player);
            [[nodiscard]] std::optional<PlayerControllerState> GetPlayerControllerState(const PlayerControllerName& player);
            [[nodiscard]] bool SetPlayerControllerMovement(const PlayerControllerName& player,
                                                           const glm::vec3& movement,
                                                           const float& minDistance);
            [[nodiscard]] bool SetPlayerControllerUpDirection(const PlayerControllerName& player, const glm::vec3& upDirUnit);
            [[nodiscard]] bool DestroyPlayerController(const PlayerControllerName& player);

            //
            // RayCasting
            //
            [[nodiscard]] std::vector<RaycastResult> RaycastForCollisions(
                const glm::vec3& rayStart_worldSpace,
                const glm::vec3& rayEnd_worldSpace) const;

            //
            // PxSimulationEventCallback
            //
            void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
            void onWake(physx::PxActor** actors, physx::PxU32 count) override;
            void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
            void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
            void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
            void onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

            //
            // Metrics
            //
            [[nodiscard]] std::size_t GetNumStaticRigidBodies() const;
            [[nodiscard]] std::size_t GetNumDynamicRigidBodies() const;

            //
            // Other
            //
            void DebugCheckResources() const;

        private:

            struct PhysxMovement
            {
                PhysxMovement(glm::vec3 _movement, float _minDistance)
                    : movement(_movement)
                    , minDistance(_minDistance)
                { }

                glm::vec3 movement;
                float minDistance;
            };

            struct PhysXRigidBody
            {
                PhysXRigidBody(RigidBody _data,
                               physx::PxRigidActor* _pRigidActor,
                               std::vector<std::pair<physx::PxShape*, physx::PxMaterial*>> _shapes)
                    : data(std::move(_data))
                    , pRigidActor(_pRigidActor)
                    , shapes(std::move(_shapes))
                { }

                RigidBody data;
                physx::PxRigidActor* pRigidActor{nullptr};
                std::vector<std::pair<physx::PxShape*, physx::PxMaterial*>> shapes;

                bool isDirty{false};
            };

            struct PhysXPlayerController
            {
                explicit PhysXPlayerController(physx::PxController* _pPxController, physx::PxMaterial* _pMaterial)
                    : pPxController(_pPxController)
                    , pMaterial(_pMaterial)
                { }

                std::optional<PhysxMovement> movementCommand;
                physx::PxController* pPxController{nullptr};
                physx::PxMaterial* pMaterial;
                std::size_t msSinceLastUpdate{0};
            };

        private:

            void ApplyPlayerControllerMovements(unsigned int timeStep);

            void SyncRigidBodyDataFromPhysX();
            static void SyncPhysXRigidBodyDataFrom(physx::PxRigidActor* pRigidActor,
                                                   const RigidActorData& actor,
                                                   const RigidBodyData& body);

            [[nodiscard]] physx::PxRigidActor* CreateRigidActor(const RigidBodyData& body);

            [[nodiscard]] physx::PxMaterial* CreateMaterial(const MaterialData& material);

            [[nodiscard]] physx::PxShape* CreateShape(const ShapeData& shape, physx::PxMaterial* pMaterial);

            [[nodiscard]] physx::PxShape* CreateShape_AABB(const ShapeData& shape,
                                                           physx::PxMaterial* pMaterial,
                                                           glm::vec3& localPositionAdjustment);

            [[nodiscard]] physx::PxShape* CreateShape_Capsule(const ShapeData& shape,
                                                              physx::PxMaterial* pMaterial,
                                                              glm::vec3& localPositionAdjustment);

            [[nodiscard]] physx::PxShape* CreateShape_Sphere(const ShapeData& shape,
                                                             physx::PxMaterial* pMaterial,
                                                             glm::vec3& localPositionAdjustment);

            [[nodiscard]] physx::PxShape* CreateShape_StaticMesh(const ShapeData& shape,
                                                                 physx::PxMaterial* pMaterial,
                                                                 glm::vec3& localPositionAdjustment);

            [[nodiscard]] static inline physx::PxRigidBody* GetAsRigidBody(const physx::PxRigidActor* pRigidActor);
            [[nodiscard]] static inline physx::PxRigidDynamic* GetAsRigidDynamic(const physx::PxRigidActor* pRigidActor);

            [[nodiscard]] std::optional<std::variant<EntityId, PlayerControllerName>> PxRigidActorToEntity(physx::PxRigidActor* pRigidActor) const;

        private:

            PhysicsSceneName m_name;
            PhysicsSceneParams m_params;
            Common::ILogger::Ptr m_logger;
            std::shared_ptr<IWorldResources> m_worldResources;
            physx::PxPhysics* m_pPhysics;
            physx::PxCpuDispatcher* m_pCpuDispatcher;

            physx::PxScene* m_pScene{nullptr};
            physx::PxControllerManager* m_pControllerManager{nullptr};
            physx::PxCudaContextManager* m_pCudaContextManager{nullptr};

            // Rigid Bodies
            std::unordered_map<EntityId, PhysXRigidBody> m_entityToRigidBody;
            std::unordered_map<physx::PxActor*, EntityId> m_physXActorToEntity;

            // Player Controllers
            std::unordered_map<PlayerControllerName, PhysXPlayerController> m_playerControllers;
            std::unordered_map<physx::PxActor*, PlayerControllerName> m_physXActorToPlayerController;

            std::queue<PhysicsTriggerEvent> m_triggerEvents;
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSXSCENE_H
