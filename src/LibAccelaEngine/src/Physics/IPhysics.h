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
#include <unordered_map>
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
             * Instructs the physics system to mark bodies as no longer dirty.
             */
            virtual void MarkBodiesClean() = 0;

            /**
             * Pops all trigger events that have occurred during SimulationSteps, since the last
             * time this method was called.
             *
             * @return A time-sorted queue of per-scene trigger events
             */
            [[nodiscard]] virtual std::unordered_map<PhysicsSceneName, std::queue<PhysicsTriggerEvent>> PopTriggerEvents() = 0;

            /**
            * @param eid The EntityId to fetch the RigidBody for
            * @param scene Optional scene identifier which can reduce an internal lookup if supplied
            *
            * @return The latest RigidBody for the corresponding eid, with a boolean specifying whether
            * the body is dirty, or std::nullopt if no such entity body exists
            */
            [[nodiscard]] virtual std::optional<std::pair<RigidBody, bool>> GetRigidBody(
                const EntityId& eid,
                const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Add a rigid body to the physics simulation
             *
             * @param scene The scene to create the body in
             * @param eid The EntityId associated with the body
             * @param rigidBody The body's definition
             */
            virtual bool CreateRigidBody(const PhysicsSceneName& scene,
                                         const EntityId& eid,
                                         const RigidBody& rigidBody) = 0;

            /**
             * Update an existing rigid body from components
             *
             * @param eid The EntityId associated with the body to be updated
             * @param rigidBody The body's definition
             * @param scene Optional scene identifier which can reduce an internal lookup if supplied
             */
            [[nodiscard]] virtual bool UpdateRigidBody(const EntityId& eid,
                                                       const RigidBody& rigidBody,
                                                       const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Removes a rigid body previously created via CreateRigidBodyFromEntity() from the physics
             * simulation.
             *
             * @param eid The EntityId associated with the previously created body
             * @param scene Optional scene identifier which can reduce an internal lookup if supplied
             */
            [[nodiscard]] virtual bool DestroyRigidBody(const EntityId& eid,
                                                        const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Resets all Physics scenes to a default state. All previously created
             * scenes will still exist, but will be reset to their default, empty,
             * state.
             */
            virtual void ClearAll() = 0;

            /**
             * Sets debug rendering of physics state on or off. Affects all scenes.
             *
             * @param enable Whether or not debug rendering is enabled
             */
            virtual void EnableDebugRenderOutput(bool enable) = 0;

            /**
             * Fetches physics debug triangles from all scenes.
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
