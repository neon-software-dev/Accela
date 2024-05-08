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
#include <unordered_set>
#include <queue>

namespace Accela::Engine
{
    class PhysXPhysics : public IPhysics, public IPhysicsRuntime, private physx::PxSimulationEventCallback
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
            [[nodiscard]] std::optional<std::pair<RigidBody, bool>> GetRigidBody(const EntityId& eid) override;
            void MarkBodiesClean() override;
            [[nodiscard]] std::queue<PhysicsTriggerEvent> PopTriggerEvents() override;

            [[nodiscard]] bool CreateRigidBody(const EntityId& eid, const RigidBody& rigidBody) override;
            [[nodiscard]] bool UpdateRigidBody(const EntityId& eid, const RigidBody& rigidBody) override;
            void DestroyRigidBody(const EntityId& eid) override;

            [[nodiscard]] bool CreatePlayerController(const std::string& name,
                                                      const glm::vec3& position,
                                                      const float& radius,
                                                      const float& height,
                                                      const PhysicsMaterial& material) override;
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

            //
            // PxSimulationEventCallback
            //
            void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
            void onWake(physx::PxActor** actors, physx::PxU32 count) override;
            void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
            void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
            void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
            void onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

        private:

            struct PhysXRigidBody
            {
                PhysXRigidBody(const RigidBody& _data,
                               physx::PxRigidActor* _pRigidActor,
                               physx::PxMaterial* _pMaterial,
                               physx::PxShape* _pShape)
                    : data(_data)
                    , pRigidActor(_pRigidActor)
                    , pMaterial(_pMaterial)
                    , pShape(_pShape)
                { }

                RigidBody data;
                physx::PxRigidActor* pRigidActor{nullptr};
                physx::PxMaterial* pMaterial{nullptr};
                physx::PxShape* pShape{nullptr};

                bool isDirty{false};
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

            void InitPhysX();
            void DestroyPhysX();

            void CreateScene();
            void DestroyScene();

            void SyncBodyDataFromPhysX();

            void ApplyPlayerControllerMovements(unsigned int timeStep);

            static void SetPhysXRigidBodyDataFrom(physx::PxRigidActor* pRigidActor,
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

            [[nodiscard]] physx::PxShape* CreateShape_HeightMap(const ShapeData& shape,
                                                                physx::PxMaterial* pMaterial,
                                                                glm::vec3& localPositionAdjustment,
                                                                glm::quat& localOrientationAdjustment);

            [[nodiscard]] static inline physx::PxRigidBody* GetAsRigidBody(const physx::PxRigidActor* pRigidActor);
            [[nodiscard]] static inline physx::PxRigidDynamic* GetAsRigidDynamic(const physx::PxRigidActor* pRigidActor);

            void SyncMetrics();
            void DebugCheckResources();

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
            physx::PxCudaContextManager* m_pxCudaContextManager{nullptr};

            // Scene specific
            physx::PxScene* m_pxScene{nullptr};
            physx::PxControllerManager* m_pxControllerManager{nullptr};

            // Rigid bodies that were created from entities via CreateRigidBody
            std::unordered_map<EntityId, PhysXRigidBody> m_entityToRigidBody;
            std::unordered_map<physx::PxActor*, EntityId> m_physXActorToEntity;

            // Player controllers created via CreatePlayerController
            std::unordered_map<std::string, PhysXPlayerController> m_playerControllers;
            std::unordered_map<physx::PxActor*, std::string> m_physXActorToPlayerController;

            std::queue<PhysicsTriggerEvent> m_triggerEvents;
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSXPHYSICS_H
