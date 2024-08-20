/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Render/Util/Rotation.h>

namespace Accela::Render
{

Rotation::Rotation(const Degrees& degrees, const glm::vec3& rotAxis, const std::optional<WorldPosition>& _rotPoint)
    : Rotation(Radians(glm::radians(degrees.value)), rotAxis, _rotPoint)
{
}

Rotation::Rotation(const Radians& radians, const glm::vec3& rotAxis, const std::optional<WorldPosition>& _rotPoint)
    : rotPoint(_rotPoint)
{
    rotation = glm::normalize(
        glm::angleAxis(radians.value, rotAxis)
    );
}

Rotation::Rotation(const glm::quat& _rotation, const std::optional<WorldPosition>& _rotPoint)
    : rotation(glm::normalize(_rotation))
    , rotPoint(_rotPoint)
{ }

glm::quat Rotation::ApplyToOrientation(const glm::quat& in) const
{
    return glm::normalize(rotation * in);
}

glm::vec3 Rotation::ApplyToPosition(const glm::vec3& in) const
{
    // If we're not rotating around a specific point, a point's position
    // will never change, only its orientation
    if (!rotPoint)
    {
        return in;
    }

    const glm::vec3 worldRotPoint = rotPoint->value;

    return worldRotPoint + (rotation * (in - worldRotPoint));
}

}
