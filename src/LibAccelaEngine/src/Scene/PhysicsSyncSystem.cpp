/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "PhysicsSyncSystem.h"

#include "../Physics/IPhysics.h"

#include "../Component/PhysicsStateComponent.h"

#include <Accela/Engine/Component/PhysicsComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>
#include <Accela/Engine/Component/BoundsComponent.h>

#include "../Metrics.h"

#include <Accela/Common/Timer.h>

namespace Accela::Engine
{

PhysicsSyncSystem::PhysicsSyncSystem(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics, IPhysicsPtr physics)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_physics(std::move(physics))
{

}

void PhysicsSyncSystem::Execute(const RunState::Ptr& runState, entt::registry& registry)
{
    Common::Timer syncSystemTimer(Engine_PhysicsSyncSystem_Time);

    PreSimulationStep(runState, registry);
    m_physics->SimulationStep(runState->timeStep);
    PostSimulationStep(runState, registry);

    syncSystemTimer.StopTimer(m_metrics);
}

void PhysicsSyncSystem::OnPhysicsStateComponentDestroyed(const EntityId& entityId)
{
    m_physics->DestroyRigidBody(entityId);
}

void PhysicsSyncSystem::PreSimulationStep(const RunState::Ptr&, entt::registry& registry) const
{
    //
    // Loop through all physics entities, and if any are New or Dirty, update the physics system
    // with the new state
    //
    registry.view<PhysicsStateComponent, PhysicsComponent, BoundsComponent, TransformComponent>().each(
        [&](const auto& eid,
            PhysicsStateComponent& physicsStateComponent,
            const PhysicsComponent& physicsComponent,
            const BoundsComponent& boundsComponent,
            const TransformComponent& transformComponent)
        {
            switch (physicsStateComponent.state)
            {
                case ComponentState::New:
                    m_physics->CreateRigidBodyFromEntity((EntityId)eid, physicsComponent, transformComponent, boundsComponent);
                break;
                case ComponentState::Dirty:
                    m_physics->UpdateRigidBodyFromEntity((EntityId)eid, physicsComponent, transformComponent);
                break;
                case ComponentState::Synced:
                    // no-op
                break;
            }

            physicsStateComponent.state = ComponentState::Synced;
        });
}

void PhysicsSyncSystem::PostSimulationStep(const RunState::Ptr&, entt::registry& registry) const
{
    //
    // Loop through all physics entities, and update their state from the physics system, now that
    // the physics state has been changed.
    //
    registry.view<PhysicsStateComponent, PhysicsComponent, BoundsComponent, TransformComponent>().each(
        [&](const auto& eid,
            const PhysicsStateComponent&,
            const PhysicsComponent& physicsComponent,
            const BoundsComponent&,
            const TransformComponent& transformComponent)
        {
            PhysicsComponent updatedPhysics = physicsComponent;
            TransformComponent updatedTransform = transformComponent;

            m_physics->PostSimulationSyncRigidBodyEntity(
                (EntityId)eid,
                updatedPhysics,
                updatedTransform
            );

            if (updatedPhysics != physicsComponent)
            {
                registry.emplace_or_replace<PhysicsComponent>(eid, updatedPhysics);
            }

            if (updatedTransform != transformComponent)
            {
                registry.emplace_or_replace<TransformComponent>(eid, updatedTransform);
            }
        }
    );
}

}
