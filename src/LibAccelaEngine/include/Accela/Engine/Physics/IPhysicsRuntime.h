/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_IPHYSICSRUNTIME_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_IPHYSICSRUNTIME_H

#include "RaycastResult.h"
#include "PlayerController.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Component/PhysicsComponent.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <expected>
#include <string>

namespace Accela::Engine
{
    struct PlayerControllerState
    {
        bool collisionAbove{false};
        bool collisionBelow{false};
    };

    /**
     * User-facing interface to the engine's physics system
     */
    class IPhysicsRuntime
    {
        public:

            using Ptr = std::shared_ptr<IPhysicsRuntime>;

        public:

            virtual ~IPhysicsRuntime() = default;

            /**
             * Manually apply a force to a body
             *
             * @param eid The entity the force should be applied to
             * @param force A vector defining the force to be applied
             *
             * @return False if the entity's physics body doesn't exist
             */
            virtual bool ApplyRigidBodyLocalForce(const EntityId& eid, const glm::vec3& force) = 0;

            /**
             * Perform a raycast for physics objects.
             *
             * @param rayStart_worldSpace World space ray start position
             * @param rayEnd_worldSpace World space ray end position
             *
             * @return A RaycastResult for each hit physics object, sorted by nearest to furthest.
             */
            [[nodiscard]] virtual std::vector<RaycastResult> RaycastForCollisions(
                const glm::vec3& rayStart_worldSpace,
                const glm::vec3& rayEnd_worldSpace) const = 0;

            /**
             * Create a PlayerController within the physics system
             *
             * @param name Unique name to identify the player controller
             * @param position The player's initial world-space position
             * @param radius The radius of the player's capsule shape
             * @param height The height of the player's capsule shape
             *
             * @return Whether the player was created successfully
             */
            [[nodiscard]] virtual bool CreatePlayerController(const std::string& name,
                                                              const glm::vec3& position,
                                                              const float& radius,
                                                              const float& height,
                                                              const PhysicsMaterial& material) = 0;

            /**
             * Returns the current world-space position of a player controller
             *
             * @param name The name that identifies the player
             *
             * @return The player's world-space position, or std::nullopt if no such player
             */
            [[nodiscard]] virtual std::optional<glm::vec3> GetPlayerControllerPosition(const std::string& name) = 0;

            /**
             * Returns physics state about a player controller
             *
             * @param name The name that identifies the player
             *
             * @return The player's physics state data, or std::nullopt if no such player
             */
            [[nodiscard]] virtual std::optional<PlayerControllerState> GetPlayerControllerState(const std::string& name) = 0;

            /**
             * Sets the current movement velocity for a player controller.
             *
             * @param name The name that identifies the player
             * @param movement Velocity vector to be used for the player's motion
             * @param minDistance Minimum distance along the velocity vector that the player can move
             *
             * @return True if the movement was applied, false if no such player
             */
            [[nodiscard]] virtual bool SetPlayerControllerMovement(const std::string& name,
                                                                   const glm::vec3& movement,
                                                                   const float& minDistance) = 0;

            /**
             * Updates what direction is considered up for a player controller
             *
             * @param name The name that identifies the player
             * @param upDirUnit Unit vector which denotes the up direction
             *
             * @return True if the update was applied, false if no such player
             */
            [[nodiscard]] virtual bool SetPlayerControllerUpDirection(const std::string& name,
                                                                      const glm::vec3& upDirUnit) = 0;

            /**
             * Destroys a previously created player controller (if it exists)
             *
             * @param name The name that identifies the player
             */
            virtual void DestroyPlayerController(const std::string& name) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_IPHYSICSRUNTIME_H
