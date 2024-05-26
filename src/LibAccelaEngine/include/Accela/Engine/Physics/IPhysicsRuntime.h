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
             * Create a new physics scene which bodies and players can be created within.
             *
             * @param scene A unique name which identifies the scene
             * @param params Parameters defining scene configuration
             *
             * @return Whether the creation was successful
             */
            [[nodiscard]] virtual bool CreateScene(const PhysicsSceneName& scene, const PhysicsSceneParams& params) = 0;

            /**
             * Destroy a previously created scene, and all resources associated with it.
             *
             * @param scene The scene to be destroyed
             *
             * @return Whether the scene was destroyed successfully
             */
            [[nodiscard]] virtual bool DestroyScene(const PhysicsSceneName& scene) = 0;

            /**
             * Manually apply a force to a body
             *
             * @param eid The entity the force should be applied to
             * @param force A vector defining the force to be applied
             * @param scene An optional name of the scene that the entity belongs to; improves performance if supplied
             *
             * @return False if the entity's physics body doesn't exist
             */
            [[nodiscard]]  virtual bool ApplyLocalForceToRigidBody(
                const EntityId& eid,
                const glm::vec3& force,
                const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Perform a raycast for scene physics objects.
             *
             * @param scene The scene in which to raycast
             * @param rayStart_worldSpace World space ray start position
             * @param rayEnd_worldSpace World space ray end position
             *
             * @return A RaycastResult for each hit physics object, sorted by nearest to furthest. An
             * empty vector if the scene doesn't exist.
             */
            [[nodiscard]] virtual std::vector<RaycastResult> RaycastForCollisions(
                const PhysicsSceneName& scene,
                const glm::vec3& rayStart_worldSpace,
                const glm::vec3& rayEnd_worldSpace) const = 0;

            /**
             * Create a PlayerController within the physics system
             *
             * @param scene The scene to create the player controller in
             * @param player Unique name to identify the player controller
             * @param position The player's initial world-space position
             * @param radius The radius of the player's capsule shape
             * @param height The height of the player's capsule shape
             *
             * @return Whether the player was created successfully
             */
            [[nodiscard]] virtual bool CreatePlayerController(const PhysicsSceneName& scene,
                                                              const PlayerControllerName& player,
                                                              const glm::vec3& position,
                                                              const float& radius,
                                                              const float& height,
                                                              const PhysicsMaterial& material) = 0;

            /**
             * Returns the current world-space position of a player controller
             *
             * @param player The name that identifies the player
             * @param scene An optional name of the scene that the player belongs to; improves performance if supplied
             *
             * @return The player's world-space position, or std::nullopt if no such player
             */
            [[nodiscard]] virtual std::optional<glm::vec3> GetPlayerControllerPosition(
                const PlayerControllerName& player,
                const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Returns physics state about a player controller
             *
             * @param player The name that identifies the player
             * @param scene An optional name of the scene that the player belongs to; improves performance if supplied
             *
             * @return The player's physics state data, or std::nullopt if no such player
             */
            [[nodiscard]] virtual std::optional<PlayerControllerState> GetPlayerControllerState(
                const PlayerControllerName& player,
                const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Sets the current movement velocity for a player controller.
             *
             * @param player The name that identifies the player
             * @param movement Velocity vector to be used for the player's motion
             * @param minDistance Minimum distance along the velocity vector that the player can move
             * @param scene An optional name of the scene that the player belongs to; improves performance if supplied
             *
             * @return True if the movement was applied, false if no such player
             */
            [[nodiscard]] virtual bool SetPlayerControllerMovement(const PlayerControllerName& player,
                                                                   const glm::vec3& movement,
                                                                   const float& minDistance,
                                                                   const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Updates what direction is considered up for a player controller
             *
             * @param player The name that identifies the player
             * @param upDirUnit Unit vector which denotes the up direction
             * @param scene An optional name of the scene that the player belongs to; improves performance if supplied
             *
             * @return True if the update was applied, false if no such player
             */
            [[nodiscard]] virtual bool SetPlayerControllerUpDirection(const PlayerControllerName& player,
                                                                      const glm::vec3& upDirUnit,
                                                                      const std::optional<PhysicsSceneName>& scene) = 0;

            /**
             * Destroys a previously created player controller (if it exists)
             *
             * @param player The name that identifies the player
             * @param scene An optional name of the scene that the player belongs to; improves performance if supplied
             */
            [[nodiscard]] virtual bool DestroyPlayerController(const PlayerControllerName& player,
                                                               const std::optional<PhysicsSceneName>& scene) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_IPHYSICSRUNTIME_H
