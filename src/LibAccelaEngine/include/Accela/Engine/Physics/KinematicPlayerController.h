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

#include <glm/glm.hpp>

#include <optional>
#include <chrono>
#include <expected>

namespace Accela::Engine
{
    class KinematicPlayerController : public PlayerController
    {
        public:

            using UPtr = std::unique_ptr<KinematicPlayerController>;

        private:

            struct Tag{};

        public:

            [[nodiscard]] static std::expected<UPtr, bool> Create(
                const std::shared_ptr<IEngineRuntime>& engine,
                const std::string& name,
                const glm::vec3& position,
                const float& radius,
                const float& height
            );

            explicit KinematicPlayerController(Tag, std::shared_ptr<IEngineRuntime> engine, std::string name);
            ~KinematicPlayerController() override;

            //
            // PlayerController
            //
            [[nodiscard]] glm::vec3 GetPosition() const override;

            void OnSimulationStep(const PlayerMovement& commandedMovement,
                                  const glm::vec3& lookUnit) override;

        private:

            struct JumpState
            {
                enum class State
                {
                    Jumping,
                    Coasting
                };

                State state{State::Jumping};

                glm::vec3 jumpVelocity{0.0f, 0.2f, 0.0f};

                std::chrono::high_resolution_clock::time_point jumpStartTime{std::chrono::high_resolution_clock::now()};
                std::optional<std::chrono::high_resolution_clock::time_point> coastStartTime;
            };

        private:

            void DestroyInternal();

            void ProcessJumpState(bool jumpCommanded);

        private:

            std::shared_ptr<IEngineRuntime> m_engine;
            std::string m_name;

            std::optional<JumpState> m_jumpState;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_KINEMATICPLAYER_H
