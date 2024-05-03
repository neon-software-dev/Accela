/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Physics/PlayerController.h>

namespace Accela::Engine
{

inline bool AreUnitVectorsParallel(const glm::vec3& a, const glm::vec3& b)
{
    return glm::abs(glm::dot(a, b)) > .9999f;
}

std::pair<glm::vec3, glm::vec3> PlayerController::GetUpAndRightUnitsFrom(const glm::vec3& lookUnit)
{
    auto upUnit = glm::vec3(0, 1, 0);

    // crossing vectors is undefined if they're parallel, so choose an alternate up
    // vector in those cases
    if (AreUnitVectorsParallel(lookUnit, upUnit))
    {
        // If looking up, then our "up" is re-adjusted to be pointing out of the screen
        if (lookUnit.y >= 0.0f)  { upUnit = glm::vec3(0,0,1); }
        // If looking down, then our "up" is re-adjusted to be pointing into the screen
        else                     { upUnit = glm::vec3(0,0,-1); }
    }

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
