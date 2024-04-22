/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef TESTDESKTOPAPP_MOVEMENTCOMMANDS_H
#define TESTDESKTOPAPP_MOVEMENTCOMMANDS_H

#include <glm/glm.hpp>

#include <array>
#include <algorithm>
#include <optional>

namespace Accela
{
    /**
     * Represents movements that the player input has commanded
     */
    struct MovementCommands
    {
        [[nodiscard]] bool Left() const { return data[0]; }
        [[nodiscard]] bool Right() const { return data[1]; }
        [[nodiscard]] bool Forward() const { return data[2]; }
        [[nodiscard]] bool Backward() const { return data[3]; }
        [[nodiscard]] bool Up() const { return data[4]; }
        [[nodiscard]] bool Down() const { return data[5]; }

        [[nodiscard]] bool AnyCommand() const {
            return std::ranges::any_of(data, [](const auto& val){ return val; });
        }

        [[nodiscard]] std::optional<glm::vec3> GetXZNormalizedVector() const
        {
            if (!AnyCommand()) { return std::nullopt; }

            glm::vec3 result(0);

            if (Left()) { result += glm::vec3{-1,0,0}; }
            if (Right()) { result += glm::vec3{1,0,0}; }
            if (Forward()) { result += glm::vec3{0,0,-1}; }
            if (Backward()) { result += glm::vec3{0,0,1}; }

            if (result == glm::vec3(0))
            {
                return std::nullopt;
            }

            return glm::normalize(result);
        }

        [[nodiscard]] std::optional<glm::vec3> GetXYZNormalizedVector() const
        {
            if (!AnyCommand()) { return std::nullopt; }

            glm::vec3 result(0);

            if (Left()) { result += glm::vec3{-1,0,0}; }
            if (Right()) { result += glm::vec3{1,0,0}; }
            if (Forward()) { result += glm::vec3{0,0,-1}; }
            if (Backward()) { result += glm::vec3{0,0,1}; }
            if (Up()) { result += glm::vec3{0,1,0}; }
            if (Down()) { result += glm::vec3{0,-1,0}; }

            if (result == glm::vec3(0))
            {
                return std::nullopt;
            }

            return glm::normalize(result);
        }

        void SetLeft() { data[0] = true; }
        void SetRight() { data[1] = true; }
        void SetForward() { data[2] = true; }
        void SetBackward() { data[3] = true; }
        void SetUp() { data[4] = true; }
        void SetDown() { data[5] = true; }

        std::array<bool, 6> data = {false, false, false, false, false, false};
    };
}

#endif //TESTDESKTOPAPP_MOVEMENTCOMMANDS_H
