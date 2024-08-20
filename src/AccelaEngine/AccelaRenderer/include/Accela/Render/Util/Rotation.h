/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_ROTATION_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_ROTATION_H

#include "Units.h"

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <utility>
#include <optional>
#include <cmath>

namespace Accela::Render
{
    /**
     * Defines a rotation operation, around an optional point
     */
    struct ACCELA_PUBLIC Rotation
    {
        Rotation() = default;

        Rotation(const Degrees& degrees,
                 const glm::vec3& rotAxis,
                 const std::optional<WorldPosition>& _rotPoint = std::nullopt);

        Rotation(const Radians& radians,
                 const glm::vec3& rotAxis,
                 const std::optional<WorldPosition>& _rotPoint = std::nullopt);

        explicit Rotation(const glm::quat& _rotation,
                          const std::optional<WorldPosition>& _rotPoint = std::nullopt);

        [[nodiscard]] glm::quat ApplyToOrientation(const glm::quat& in) const;
        [[nodiscard]] glm::vec3 ApplyToPosition(const glm::vec3& in) const;

        glm::quat rotation{glm::identity<glm::quat>()};
        std::optional<WorldPosition> rotPoint;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_ROTATION_H
