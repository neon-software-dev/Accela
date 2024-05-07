/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_PHYSICSSYNCSYSTEM_H
#define LIBACCELAENGINE_SRC_SCENE_PHYSICSSYNCSYSTEM_H

#include "IWorldSystem.h"

#include "../ForwardDeclares.h"
#include "../Physics/RigidBody.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Component/PhysicsComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>
#include <Accela/Engine/Component/BoundsComponent.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

namespace Accela::Engine
{
    class PhysicsSyncSystem : public IWorldSystem
    {
        public:

            PhysicsSyncSystem(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics, IPhysicsPtr physics);

            [[nodiscard]] Type GetType() const noexcept override { return Type::PhysicsSync; };

            void Execute(const RunState::Ptr& runState, entt::registry& registry) override;

            void OnPhysicsStateComponentDestroyed(const EntityId& entityId);

        private:

            void PreSimulationStep(const RunState::Ptr& runState, entt::registry& registry) const;
            void PostSimulationStep(const RunState::Ptr& runState, entt::registry& registry) const;

            [[nodiscard]] static RigidBody GetRigidBodyFrom(const PhysicsComponent& physicsComponent,
                                                            const BoundsComponent& boundsComponent,
                                                            const TransformComponent& transformComponent);
            [[nodiscard]] static MaterialData GetMaterial(const PhysicsComponent& physicsComponent);
            [[nodiscard]] static ShapeData GetShape(const MaterialData& material,
                                                    const BoundsComponent& boundsComponent,
                                                    const TransformComponent& transformComponent);

             static void SetComponentsFromData(const RigidBody& data,
                                               PhysicsComponent& physicsComponent,
                                               TransformComponent& transformComponent);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IPhysicsPtr m_physics;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_PHYSICSSYNCSYSTEM_H
