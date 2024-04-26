/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_PHYSICS_H
#define LIBACCELAENGINE_SRC_PHYSICS_PHYSICS_H

#include "IPhysics.h"
#include "ReactPhysics3d.h"

#include "../ForwardDeclares.h"

#include "../Scene/HeightMapData.h"

#include <Accela/Engine/Physics/IPhysicsRuntime.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <unordered_map>
#include <expected>

namespace Accela::Engine
{
    class Physics : public IPhysics, public IPhysicsRuntime
    {
        public:

            Physics(Common::ILogger::Ptr logger,
                    Common::IMetrics::Ptr metrics,
                    IWorldResourcesPtr worldResources);
            ~Physics() override;

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

            struct RigidBody
            {
                RigidBody(reactphysics3d::RigidBody* _pBody, reactphysics3d::Collider* _pCollider)
                    : pBody(_pBody)
                    , pCollider(_pCollider)
                { }

                reactphysics3d::RigidBody* pBody{nullptr};
                reactphysics3d::Collider* pCollider{nullptr};
            };

        private:

            void CreateWorld();
            void DestroyAll();

            static void SyncRigidBodyData(reactphysics3d::RigidBody* pBody,
                                          const PhysicsComponent& physicsComponent,
                                          const TransformComponent& transformComponent);

            [[nodiscard]] static reactphysics3d::Transform GetRP3DTransform(const TransformComponent& transformComponent);
            [[nodiscard]] std::expected<reactphysics3d::Collider*, bool> AddRigidCollider(reactphysics3d::RigidBody* pBody,
                                                                                          const PhysicsComponent& physicsComponent,
                                                                                          const BoundsComponent& boundsComponent,
                                                                                          const TransformComponent& transformComponent);

            [[nodiscard]] std::expected<reactphysics3d::CollisionShape*, bool> CreateCollisionShapeAABB(
                const BoundsComponent& boundsComponent,
                const TransformComponent& transformComponent,
                glm::vec3& localPositionAdjustment);

            [[nodiscard]] std::expected<reactphysics3d::CollisionShape*, bool> CreateCollisionShapeSphere(
                const BoundsComponent& boundsComponent,
                const TransformComponent& transformComponent,
                glm::vec3& localPositionAdjustment);

            [[nodiscard]] std::expected<reactphysics3d::CollisionShape*, bool> CreateCollisionShapeCapsule(
                const BoundsComponent& boundsComponent,
                const TransformComponent& transformComponent,
                glm::vec3& localPositionAdjustment);

            [[nodiscard]] std::expected<reactphysics3d::CollisionShape*, bool> CreateCollisionShapeHeightMap(
                const BoundsComponent& boundsComponent,
                const TransformComponent& transformComponent,
                glm::vec3& localPositionAdjustment);

            void SyncMetrics();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResourcesPtr m_worldResources;

            reactphysics3d::PhysicsCommon m_physicsCommon;
            reactphysics3d::PhysicsWorld* m_pPhysicsWorld{nullptr};

            std::unordered_map<EntityId, RigidBody> m_entityToRigidBody;
            std::unordered_map<rp3d::CollisionBody*, EntityId> m_bodyToEntity;

            bool m_debugRendering{false};
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSICS_H
