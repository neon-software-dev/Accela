/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Camera2D.h>

namespace Accela::Engine
{

glm::vec3 Camera2D::GetPosition() const
{
    return m_position;
}

void Camera2D::TranslateBy(const glm::vec2& translation) noexcept
{
    m_position += glm::vec3(translation, 0.0f);
    EnforceBounds();
}

glm::vec3 Camera2D::GetLookUnit() const
{
    return {0,0,1};
}

glm::vec3 Camera2D::GetUpUnit() const
{
    return {0,-1,0};
}

glm::vec3 Camera2D::GetRightUnit() const
{
    return glm::normalize(glm::cross(GetLookUnit(), GetUpUnit()));
}

void Camera2D::SetPosition(const glm::vec2& position) noexcept
{
    m_position = glm::vec3(position, 0.0f);
    EnforceBounds();
}

void Camera2D::SetBounds(const glm::vec2& topLeft, const glm::vec2& bottomRight)
{
    m_topLeftBound =  topLeft;
    m_bottomRightBound = bottomRight;

    EnforceBounds();
}

void Camera2D::EnforceBounds()
{
    if (!m_topLeftBound || !m_bottomRightBound) { return; }

    m_position.x = std::max(m_topLeftBound->x, m_position.x);
    m_position.x = std::min(m_bottomRightBound->x, m_position.x);

    m_position.y = std::max(m_topLeftBound->y, m_position.y);
    m_position.y = std::min(m_bottomRightBound->y, m_position.y);
}

}
