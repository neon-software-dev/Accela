/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_PLAYERCONTROLLER_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_PLAYERCONTROLLER_H

#include <Accela/Common/Build.h>

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <utility>

namespace Accela::Engine
{
    struct PlayerMovement
    {
        bool left{false};
        bool right{false};
        bool forward{false};
        bool backward{false};
        bool up{false};
        bool down{false};

        [[nodiscard]] bool AnyCommand() const {
            return left || right || forward || backward || up || down;
        }
    };

    class PlayerController
    {
        public:

            using Ptr = std::shared_ptr<PlayerController>;
            using UPtr = std::unique_ptr<PlayerController>;

        public:

            virtual ~PlayerController() = default;

            [[nodiscard]] static std::pair<glm::vec3, glm::vec3> GetUpAndRightUnitsFrom(const glm::vec3& lookUnit);
            [[nodiscard]] static std::optional<glm::vec3> GetNormalizedXZVector(const PlayerMovement& movement);
            [[nodiscard]] static std::optional<glm::vec3> GetNormalizedXYZVector(const PlayerMovement& movement);

            [[nodiscard]] virtual glm::vec3 GetPosition() const = 0;

            virtual void OnSimulationStep(const PlayerMovement& commandedMovement,
                                          const glm::vec3& lookUnit) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_PLAYERCONTROLLER_H
