/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef TESTDESKTOPAPP_PLAYER_H
#define TESTDESKTOPAPP_PLAYER_H

#include "MovementCommands.h"

#include <Accela/Engine/IEngineRuntime.h>
#include <Accela/Engine/Camera.h>
#include <Accela/Engine/Entity/SceneEntity.h>
#include <Accela/Engine/Component/Components.h>

#include <Accela/Render/Id.h>

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <utility>

namespace Accela
{
    class Player : public Engine::SceneEntity
    {
        public:

            using UPtr = std::unique_ptr<Player>;

        private:

            struct Tag{};

        public:

            static UPtr Create(const Engine::IEngineRuntime::Ptr& engine,
                               const std::string& sceneName,
                               const Engine::SceneEvents::Ptr& sceneEvents,
                               const glm::vec3& position,
                               const std::optional<Render::MeshId>& playerMeshId = std::nullopt,
                               const std::optional<Render::MaterialId>& playerMaterialId = std::nullopt);

            Player(Tag,
                   Engine::IEngineRuntime::Ptr engine,
                   std::string sceneName,
                   Engine::SceneEvents::Ptr sceneEvents,
                   const Engine::EntityId& eid);

            ~Player() override;

            void Destroy() override;

            [[nodiscard]] glm::vec3 GetPosition() const noexcept;
            [[nodiscard]] glm::vec3 GetEyesPosition() const noexcept;

            void OnMovementCommanded(const glm::vec3& xzInput, const glm::vec3& lookUnit);

            // TODO: This should come from OnMovement, e.g. holding down space should jump higher
            void OnJumpCommanded();

            void OnSimulationStep(const Engine::IEngineRuntime::Ptr& engine, unsigned int timeStep) override;

        private:

            // TODO: If in the end we don't need to do anything special on slope, combine ground and slope
            enum class LocationState
            {
                Ground,
                Slope,
                Air
            };

            struct GroundRaycast
            {
                GroundRaycast(float _distanceFromCapsule, const Engine::RaycastResult& _raycast)
                    : distanceFromCapsule(_distanceFromCapsule)
                    , raycast(_raycast)
                { }

                // Distance from the ground to the nearest point on the player capsule along the raycast
                float distanceFromCapsule;

                // The engine raycast that intersected with the ground
                Engine::RaycastResult raycast;
            };

            struct PlayerBounds
            {
                PlayerBounds() = default;

                PlayerBounds(const glm::vec3& _position, const glm::vec3& _topCenter, const glm::vec3& _bottomCenter)
                    : position(_position)
                    , topCenter(_topCenter)
                    , bottomCenter(_bottomCenter)
                {}

                glm::vec3 position{0};
                glm::vec3 topCenter{0};
                glm::vec3 bottomCenter{0};
            };

        private:

            void DestroyInternal();

            void SyncCurrentState();

            void ApplyAntiGrav(float gravPercent);
            void EnforceSpeedLimit();
            void UpdateMetrics();

            [[nodiscard]] std::optional<Engine::TransformComponent> GetTransformComponent() const;
            [[nodiscard]] std::optional<Engine::PhysicsComponent> GetPhysicsComponent() const;
            [[nodiscard]] static std::pair<glm::vec3, glm::vec3> GetUpAndRightFrom(const glm::vec3& lookUnit);
            [[nodiscard]] static PlayerBounds GetPlayerBounds(const glm::vec3& playerPosition);

            [[nodiscard]] bool IsTouchingGround() const;

            [[nodiscard]] static glm::vec3 MapMovementToLookPlane(const glm::vec3& xzInput, const glm::vec3& lookUnit);

            /**
             * Casts a ray down through the player capsule at a given x/z offset from the center of the capsule,
             * attempting to find a solid surface underneath the capsule.
             *
             * @param playerBounds The bounds of the player capsule
             * @param rayXZOffset The x/z offset to cast the ray at
             *
             * @return A GroundRaycast if a surface was impacted within the length of the ray that was cast
             */
            [[nodiscard]] std::optional<GroundRaycast> FindGroundByRayOffset(const PlayerBounds& playerBounds,
                                                                             const glm::vec2& rayXZOffset) const;

            /**
             * Attempts to locate ground / solid surface underneath the player capsule by casting rays down through the
             * capsule through its center and perimeter.
             *
             * @param playerBounds The bounds of the player capsule
             * @param numPerimeterTestPoints The number (>= 1) of equidistant points along the outer perimeter of
             * the capsule to also raycast down (to detect ground that's underneath an edge of the capsule but not
             * the exact center).
             *
             * @return A GroundRaycast, if a surface was impacted within the length of a ray that was cast. If multiple
             * rays impacted surface, returns the ray with the smallest distance between the impact and the capsule.
             */
            [[nodiscard]] std::optional<GroundRaycast> RaycastForGround(const PlayerBounds& playerBounds,
                                                                        unsigned int numPerimeterTestPoints) const;

            /**
             * Determines, from a GroundRaycast, whether the player capsule is in the air, on flat(ish) ground, or
             * on sloped ground.
             */
            [[nodiscard]] static LocationState DetermineLocationState(const std::optional<GroundRaycast>& groundRaycast);

        private:

            Engine::EntityId m_eid;

            PlayerBounds m_playerBounds{};
            std::optional<GroundRaycast> m_groundRaycast;
            LocationState m_locationState{LocationState::Ground};
    };
}

#endif //TESTDESKTOPAPP_PLAYER_H
