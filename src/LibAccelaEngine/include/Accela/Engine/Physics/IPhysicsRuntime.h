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

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <expected>
#include <string>

namespace Accela::Engine
{
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

            [[nodiscard]] virtual bool CreatePlayerController(const std::string& name,
                                                              const glm::vec3& position,
                                                              const float& radius,
                                                              const float& height) = 0;

            [[nodiscard]] virtual std::optional<glm::vec3> GetPlayerControllerPosition(const std::string& name) = 0;

            [[nodiscard]] virtual bool SetPlayerControllerMovement(const std::string& name,
                                                                   const glm::vec3& movement,
                                                                   const float& minDistance) = 0;

    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_IPHYSICSRUNTIME_H
