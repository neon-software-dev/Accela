#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_ROTATION_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_ROTATION_H

#include "Units.h"

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
    struct Rotation
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
