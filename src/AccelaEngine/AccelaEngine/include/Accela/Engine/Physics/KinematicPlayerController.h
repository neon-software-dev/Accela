/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_KINEMATICPLAYER_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_KINEMATICPLAYER_H

#include <Accela/Engine/Scene/SceneCommon.h>
#include <Accela/Engine/Physics/PlayerController.h>
#include <Accela/Engine/Entity/SceneEntity.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <optional>
#include <chrono>
#include <expected>

namespace Accela::Engine
{
    enum class KinematicLocationState
    {
        /** The player is standing on a surface */
        Surface,

        /** The player is in the air */
        Air
    };

    enum class KinematicJumpState
    {
        /** The player is actively jumping upwards */
        Jumping,

        /** The player is no longer actively jumping but is still coasting to the top of their jump arc */
        Coasting,

        /** The player is no longer actively jumping and is now free falling downwards */
        FreeFall
    };

    class ACCELA_PUBLIC KinematicPlayerController : public PlayerController
    {
        public:

            using UPtr = std::unique_ptr<KinematicPlayerController>;

        private:

            struct Tag{};

        public:

            [[nodiscard]] static std::expected<UPtr, bool> Create(
                const std::shared_ptr<IEngineRuntime>& engine,
                const PhysicsSceneName& scene,
                const PlayerControllerName& name,
                const glm::vec3& position,
                const float& radius,
                const float& height
            );

            explicit KinematicPlayerController(Tag,
                                               std::shared_ptr<IEngineRuntime> engine,
                                               PhysicsSceneName scene,
                                               PlayerControllerName name);
            ~KinematicPlayerController() override;

            /**
             * @return Whether the player is standing on a surface or in the air
             */
            [[nodiscard]] KinematicLocationState GetLocationState() const;

            /**
             * @return The current state of the player's jump, or std::nullopt if
             * they're not jumping
             */
            [[nodiscard]] std::optional<KinematicJumpState> GetJumpState() const;

            //
            // PlayerController
            //
            [[nodiscard]] glm::vec3 GetPosition() const override;

            void OnSimulationStep(const PlayerMovement& commandedMovement,
                                  const glm::vec3& lookUnit) override;

        private:

            struct JumpState
            {
                KinematicJumpState state{KinematicJumpState::Jumping};
                std::chrono::high_resolution_clock::time_point jumpStartTime{std::chrono::high_resolution_clock::now()};
                float jumpSpeed{0.0f};
            };

        private:

            void DestroyInternal();

            [[nodiscard]] static KinematicLocationState CalculateLocationState(const PlayerControllerState& playerControllerState);
            [[nodiscard]] static std::optional<JumpState> CalculateJumpState(
                const PlayerControllerState& playerControllerState,
                const std::optional<JumpState>& previousJumpState,
                bool jumpCommanded
            );

            [[nodiscard]] glm::vec3 CalculatePlayerVelocity(const PlayerMovement& commandedMovement,
                                                            const glm::vec3& lookUnit) const;

        private:

            std::shared_ptr<IEngineRuntime> m_engine;
            PhysicsSceneName m_scene;
            PlayerControllerName m_name;

            KinematicLocationState m_locationState{KinematicLocationState::Surface};
            std::optional<JumpState> m_currentJumpState;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_KINEMATICPLAYER_H
