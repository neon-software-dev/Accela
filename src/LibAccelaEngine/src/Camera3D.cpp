/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Camera3D.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace Accela::Engine
{

Camera3D::Camera3D(const glm::vec3& position, float fovYDegrees)
    : m_fovy(fovYDegrees)
    , m_position(position)
{

}

glm::vec3 Camera3D::GetPosition() const
{
    return m_position;
}

glm::vec3 Camera3D::GetLookUnit() const
{
    return m_lookUnit;
}

glm::vec3 Camera3D::GetUpUnit() const
{
    auto upUnit = glm::vec3(0,1,0);

    const bool upParallelToLook = glm::abs(glm::dot(upUnit, m_lookUnit)) > .9999f;

    if (upParallelToLook)
    {
        // If looking up, then our "up" is re-adjusted to be pointing out of the screen
        if (m_lookUnit.y >= 0.0f)  { upUnit = glm::vec3(0,0,1); }
        // If looking down, then our "up" is re-adjusted to be pointing into the screen
        else                        { upUnit = glm::vec3(0,0,-1); }
    }

    return upUnit;
}

glm::vec3 Camera3D::GetRightUnit() const
{
    return glm::normalize(glm::cross(m_lookUnit, GetUpUnit()));
}

void Camera3D::TranslateBy(const glm::vec3& translation)
{
    glm::vec3 zTranslation = m_lookUnit * translation.z * -1.0f;
    glm::vec3 yTranslation = GetUpUnit() * translation.y;
    glm::vec3 xTranslation = GetRightUnit() * translation.x;

    m_position += zTranslation + yTranslation + xTranslation;
}

void Camera3D::SetPosition(const glm::vec3& position) noexcept
{
    m_position = position;
}

void Camera3D::RotateBy(float xRotDeg, float yRotDeg)
{
    const glm::mat4 lookRotation =
        glm::rotate(yRotDeg, GetUpUnit()) *
        glm::rotate(xRotDeg, GetRightUnit());

    m_lookUnit = glm::normalize(glm::mat3(lookRotation) * m_lookUnit);
}

float Camera3D::GetFovYDegrees() const noexcept
{
    return m_fovy;
}

void Camera3D::SetFovYDegrees(float fovy) noexcept
{
    m_fovy = fovy;
}

}
