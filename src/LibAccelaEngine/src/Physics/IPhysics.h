/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_IPHYSICS_H
#define LIBACCELAENGINE_SRC_PHYSICS_IPHYSICS_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Component/PhysicsComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>
#include <Accela/Engine/Component/BoundsComponent.h>
#include <Accela/Engine/Physics/PhysicsCommon.h>

#include <Accela/Render/Util/Triangle.h>

#include <glm/glm.hpp>

#include <vector>

namespace Accela::Engine
{
    class IPhysics
    {
        public:

            virtual ~IPhysics() = default;

            /**
             * Advance the physics simulation forward by the given timeStep of time
             *
             * @param timeStep The timeStep to simulate
             */
            virtual void SimulationStep(unsigned int timeStep) = 0;

            /**
             * Called to sync the latest state from the physics system back into an Entity's components,
             * after one or more calls to SimulationStep() have been performed.
             *
             * @param eid The EntityId of the Entity to be synced from the physics system
             * @param physicsComponent The entity's PhysicsComponent
             * @param transformComponent The entity's TransformComponent
             */
            virtual void PostSimulationSyncRigidBodyEntity(const EntityId& eid,
                                                           PhysicsComponent& physicsComponent,
                                                           TransformComponent& transformComponent) = 0;

            /**
             * Add a rigid body to the physics simulation
             *
             * @param eid The EntityId associated with the body
             * @param physicsComponent The PhysicsComponent defining the body's physics properties
             * @param transformComponent The TransformComponent defining the body's positioning
             * @param boundsComponent The BoundsComponent defining the body's physical bounds
             */
            virtual void CreateRigidBodyFromEntity(const EntityId& eid,
                                                   const PhysicsComponent& physicsComponent,
                                                   const TransformComponent& transformComponent,
                                                   const BoundsComponent& boundsComponent) = 0;

            /**
             * Update an existing rigid body from components
             *
             * @param eid The EntityId associated with the body to be updated
             * @param physicsComponent The PhysicsComponent defining the body's physics properties
             * @param transformComponent The TransformComponent defining the body's positioning
             */
            virtual void UpdateRigidBodyFromEntity(const EntityId& eid,
                                                   const PhysicsComponent& physicsComponent,
                                                   const TransformComponent& transformComponent) = 0;

            /**
             * Removes a rigid body previously created via CreateRigidBodyFromEntity() from the physics
             * simulation.
             *
             * @param eid The EntityId associated with the previously created body
             */
            virtual void DestroyRigidBody(const EntityId& eid) = 0;

            /**
             * Resets the Physics system to a default state
             */
            virtual void ClearAll() = 0;

            /**
             * Sets debug rendering of physics state on or off
             *
             * @param enable Whether or not debug rendering is enabled
             */
            virtual void EnableDebugRenderOutput(bool enable) = 0;

            /**
             * Fetches debug triangles which display the physics system.
             *
             * Requires EnableDebugRenderOutput(true) to have previously been called, or else
             * returns an empty vector.
             *
             * @return Debug triangles to be rendered
             */
            [[nodiscard]] virtual std::vector<Render::Triangle> GetDebugTriangles() const = 0;
    };
}

#endif //LIBACCELAENGINE_SRC_PHYSICS_IPHYSICS_H
