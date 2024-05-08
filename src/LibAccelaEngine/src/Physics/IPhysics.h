/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_IPHYSICS_H
#define LIBACCELAENGINE_SRC_PHYSICS_IPHYSICS_H

#include "RigidBody.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Component/PhysicsComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>
#include <Accela/Engine/Physics/PhysicsCommon.h>

#include <Accela/Render/Util/Triangle.h>

#include <glm/glm.hpp>

#include <vector>
#include <unordered_set>
#include <utility>
#include <queue>

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
             * @return The latest RigidBody for the corresponding eid, with a boolean specifying whether
             * the body is dirty, or std::nullopt if no such entity body exists
             */
            [[nodiscard]] virtual std::optional<std::pair<RigidBody, bool>> GetRigidBody(const EntityId& eid) = 0;

            /**
           * Instructs the physics system to mark bodies as no longer dirty. Call this
           * after syncing to its data after a simulation step
           */
            virtual void MarkBodiesClean() = 0;

            /**
             * Pop all trigger events that have occurred during SimulationSteps, since the last
             * time this method was called.
             *
             * @return A time-sorted queue of trigger events
             */
            [[nodiscard]] virtual std::queue<PhysicsTriggerEvent> PopTriggerEvents() = 0;

            /**
             * Add a rigid body to the physics simulation
             *
             * @param eid The EntityId associated with the body
             * @param rigidBody The body's definition
             */
            virtual bool CreateRigidBody(const EntityId& eid, const RigidBody& rigidBody) = 0;

            /**
             * Update an existing rigid body from components
             *
             * @param eid The EntityId associated with the body to be updated
             * @param rigidBody The body's definition
             */
            virtual bool UpdateRigidBody(const EntityId& eid, const RigidBody& rigidBody) = 0;

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
