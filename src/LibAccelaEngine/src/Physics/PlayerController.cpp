/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Physics/PlayerController.h>

#include <Accela/Render/Util/Vector.h>

namespace Accela::Engine
{

inline bool AreUnitVectorsParallel(const glm::vec3& a, const glm::vec3& b)
{
    return glm::abs(glm::dot(a, b)) > .9999f;
}

std::pair<glm::vec3, glm::vec3> PlayerController::GetUpAndRightUnitsFrom(const glm::vec3& lookUnit)
{
    // TODO!: Use up as reported from physics system
    auto upUnit = Render::This({0,1,0}).ButIfParallelWith(lookUnit).Then({0,0,1});

    const auto rightUnit = glm::normalize(glm::cross(lookUnit, upUnit));

    upUnit = glm::normalize(glm::cross(rightUnit, lookUnit));

    return std::make_pair(upUnit, rightUnit);
}

std::optional<glm::vec3> PlayerController::GetNormalizedXZVector(const PlayerMovement& movement)
{
    if (!movement.AnyCommand()) { return std::nullopt; }

    glm::vec3 result(0);

    if (movement.left) { result += glm::vec3{-1,0,0}; }
    if (movement.right) { result += glm::vec3{1,0,0}; }
    if (movement.forward) { result += glm::vec3{0,0,-1}; }
    if (movement.backward) { result += glm::vec3{0,0,1}; }

    if (result == glm::vec3(0))
    {
        return std::nullopt;
    }

    return glm::normalize(result);
}

std::optional<glm::vec3> PlayerController::GetNormalizedXYZVector(const PlayerMovement& movement)
{
    if (!movement.AnyCommand()) { return std::nullopt; }

    glm::vec3 result(0);

    if (movement.left) { result += glm::vec3{-1,0,0}; }
    if (movement.right) { result += glm::vec3{1,0,0}; }
    if (movement.forward) { result += glm::vec3{0,0,-1}; }
    if (movement.backward) { result += glm::vec3{0,0,1}; }
    if (movement.up) { result += glm::vec3{0,1,0}; }
    if (movement.down) { result += glm::vec3{0,-1,0}; }

    if (result == glm::vec3(0))
    {
        return std::nullopt;
    }

    return glm::normalize(result);
}

}
