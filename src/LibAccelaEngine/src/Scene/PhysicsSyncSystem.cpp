/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PhysicsSyncSystem.h"

#include "../Physics/IPhysics.h"

#include "../Component/PhysicsStateComponent.h"

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
                    m_physics->CreateRigidBody(
                        (EntityId)eid,
                        GetRigidBodyFrom(physicsComponent, boundsComponent, transformComponent)
                    );
                break;
                case ComponentState::Dirty:
                    m_physics->UpdateRigidBody(
                        (EntityId)eid,
                        GetRigidBodyFrom(physicsComponent, boundsComponent, transformComponent)
                    );
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
    // Loop through all the dirty physics entities, and update their state from the physics system, now that
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

            const auto body = m_physics->GetRigidBody((EntityId)eid);
            if (!body)
            {
                m_logger->Log(Common::LogLevel::Error,
                  "PhysicsSyncSystem::PostSimulationStep: No such entity body exists: {}", (EntityId)eid);
                return;
            }

            // If the body isn't dirty, skip it
            if (!body->second)
            {
                return;
            }

            SetComponentsFromData(body->first, updatedPhysics, updatedTransform);

            registry.emplace_or_replace<PhysicsComponent>(eid, updatedPhysics);
            registry.emplace_or_replace<TransformComponent>(eid, updatedTransform);
        }
    );

    //
    // Tell the physics system we've synced to its dirty body data
    //
    m_physics->MarkBodiesClean();
}

RigidBody PhysicsSyncSystem::GetRigidBodyFrom(const PhysicsComponent& physicsComponent,
                                              const BoundsComponent& boundsComponent,
                                              const TransformComponent& transformComponent)
{
    const auto material = GetMaterial(physicsComponent);
    const auto shape = GetShape(material, boundsComponent, transformComponent);

    std::variant<RigidBodyStaticData, RigidBodyDynamicData> subData;

    switch (physicsComponent.bodyType)
    {
        case RigidBodyType::Static:
        {
            RigidBodyStaticData rigidBodyStaticData{};

            subData = rigidBodyStaticData;
        }
        break;
        case RigidBodyType::Kinematic:
        case RigidBodyType::Dynamic:
        {
            RigidBodyDynamicData rigidBodyDynamicData{};
            rigidBodyDynamicData.linearVelocity = physicsComponent.linearVelocity;
            rigidBodyDynamicData.linearDamping = physicsComponent.linearDamping;
            rigidBodyDynamicData.angularDamping = physicsComponent.angularDamping;
            rigidBodyDynamicData.axisMotionAllowed = physicsComponent.axisMotionAllowed;

            subData = rigidBodyDynamicData;
        }
        break;
    }

    RigidBodyData rigidBodyData(physicsComponent.bodyType, subData);
    rigidBodyData.mass = physicsComponent.mass;

    const RigidActorData rigidActorData(
        shape,
        transformComponent.GetPosition(),
        transformComponent.GetOrientation()
    );

    return RigidBody(rigidActorData, rigidBodyData);
}

MaterialData PhysicsSyncSystem::GetMaterial(const PhysicsComponent& physicsComponent)
{
    MaterialData material{};
    material.staticFriction = physicsComponent.material.staticFriction;
    material.dynamicFriction = physicsComponent.material.dynamicFriction;
    material.restitution = physicsComponent.material.restitution;

    return material;
}

ShapeData PhysicsSyncSystem::GetShape(const MaterialData& material,
                                      const BoundsComponent& boundsComponent,
                                      const TransformComponent& transformComponent)
{
    return ShapeData(
        boundsComponent.bounds,
        material,
        transformComponent.GetScale(),
        boundsComponent.localTransform,
        boundsComponent.localOrientation
    );
}

void PhysicsSyncSystem::SetComponentsFromData(const RigidBody& data,
                                              PhysicsComponent& physicsComponent,
                                              TransformComponent& transformComponent)
{
    physicsComponent.bodyType = data.body.type;
    physicsComponent.mass = data.body.mass;

    switch (data.body.type)
    {
        case RigidBodyType::Static:
        {
            const auto& subData = std::get<RigidBodyStaticData>(data.body.subData);
            (void)subData;
        }
        break;
        case RigidBodyType::Kinematic:
        case RigidBodyType::Dynamic:
        {
            const auto& subData = std::get<RigidBodyDynamicData>(data.body.subData);
            physicsComponent.linearVelocity = subData.linearVelocity;
            physicsComponent.linearDamping = subData.linearDamping;
            physicsComponent.angularDamping = subData.angularDamping;
            physicsComponent.axisMotionAllowed = subData.axisMotionAllowed;
        }
        break;
    }

    physicsComponent.material.staticFriction = data.actor.shape.material.staticFriction;
    physicsComponent.material.dynamicFriction = data.actor.shape.material.dynamicFriction;
    physicsComponent.material.restitution = data.actor.shape.material.restitution;

    transformComponent.SetPosition(data.actor.position);
    transformComponent.SetOrientation(data.actor.orientation);
    transformComponent.SetScale(data.actor.shape.scale);
}

}
