/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FrustumProjection.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cassert>

namespace Accela::Render
{

std::expected<Projection::Ptr, bool> FrustumProjection::From(const RenderCamera& renderCamera, float nearDistance, float farDistance)
{
    // Near and far must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    return From(renderCamera.fovYDegrees, renderCamera.aspectRatio, nearDistance, farDistance);
}

std::expected<Projection::Ptr, bool> FrustumProjection::From(float fovYDegrees, float aspectRatio, float nearDistance, float farDistance)
{
    // Aspect ratio must be non-zero positive
    if(aspectRatio <= 0.0f) { return std::unexpected(false); }

    // fovYDegrees must be in the range (0.0, 180.0]
    if(fovYDegrees <= 0.0f || fovYDegrees > 180.0f) { return std::unexpected(false); }

    // Near and far must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    const float fovY = glm::radians(fovYDegrees);
    const float fovX = std::atan(std::tan(fovY/2.0f) * aspectRatio) * 2.0f;

    assert(fovX <= glm::radians(180.0f));
    assert(fovY <= glm::radians(180.0f));

    const float halfNearX = std::tan(fovX / 2.0f) * nearDistance;
    const float halfNearY = std::tan(fovY / 2.0f) * nearDistance;

    const auto nearMin = glm::vec3(-halfNearX, -halfNearY, -nearDistance);
    const auto nearMax = glm::vec3(halfNearX, halfNearY, -nearDistance);

    const float halfFarX = std::tan(fovX / 2.0f) * farDistance;
    const float halfFarY = std::tan(fovY / 2.0f) * farDistance;

    const auto farMin = glm::vec3(-halfFarX, -halfFarY, -farDistance);
    const auto farMax = glm::vec3(halfFarX, halfFarY, -farDistance);

    return std::make_shared<FrustumProjection>(Tag{}, nearMin, nearMax, farMin, farMax);
}

std::expected<Projection::Ptr, bool> FrustumProjection::From(const glm::vec3& farMin, const glm::vec3& farMax, float nearDistance)
{
    // Far points must be on x/z plane
    if (farMin.z != farMax.z) { return std::unexpected(false); }

    // Near must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }

    // Far points must be further than nearDistance
    if (-farMin.z <= nearDistance) { return std::unexpected(false); }

    const float farWidth = farMax.x - farMin.x;
    const float farHeight = farMax.y - farMin.y;
    const float aspectRatio = farWidth / farHeight;
    const float fovYDegrees = glm::degrees(2.0f * std::atan((farHeight / 2.0f) / -farMax.z));

    return From(fovYDegrees, aspectRatio, nearDistance, -farMax.z);
}

std::expected<Projection::Ptr, bool> FrustumProjection::FromTanHalfAngles(float leftTanHalfAngle,
                                                                          float rightTanHalfAngle,
                                                                          float topTanHalfAngle,
                                                                          float bottomTanHalfAngle,
                                                                          float nearDistance,
                                                                          float farDistance)
{
    // Near and far must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    const float leftNear = leftTanHalfAngle * nearDistance;
    const float rightNear = rightTanHalfAngle * nearDistance;
    const float topNear = topTanHalfAngle * nearDistance;
    const float bottomNear = bottomTanHalfAngle * nearDistance;

    const auto nearMin = glm::vec3(leftNear, bottomNear, -nearDistance);
    const auto nearMax = glm::vec3(rightNear, topNear, -nearDistance);

    const float leftFar = leftTanHalfAngle * farDistance;
    const float rightFar = rightTanHalfAngle * farDistance;
    const float topFar = topTanHalfAngle * farDistance;
    const float bottomFar = bottomTanHalfAngle * farDistance;

    const auto farMin = glm::vec3(leftFar, bottomFar, -farDistance);
    const auto farMax = glm::vec3(rightFar, topFar, -farDistance);

    return std::make_shared<FrustumProjection>(Tag{}, nearMin, nearMax, farMin, farMax);
}

FrustumProjection::FrustumProjection(Tag,
                                     const glm::vec3& nearMin,
                                     const glm::vec3& nearMax,
                                     const glm::vec3& farMin,
                                     const glm::vec3& farMax)
    : m_nearMin(nearMin)
    , m_nearMax(nearMax)
    , m_farMin(farMin)
    , m_farMax(farMax)
{
    assert(nearMin.z < 0.0f); assert(farMin.z < 0.0f);

    m_leftTanHalfAngle = m_nearMin.x / -m_nearMin.z;
    m_rightTanHalfAngle = m_nearMax.x / -m_nearMax.z;
    m_topTanHalfAngle = m_nearMax.y / -m_nearMax.z;
    m_bottomTanHalfAngle = m_nearMin.y / -m_nearMin.z;

    ComputeAncillary();
}

Projection::Ptr FrustumProjection::Clone() const
{
    return std::make_shared<FrustumProjection>(Tag{}, m_nearMin, m_nearMax, m_farMin, m_farMax);
}

glm::mat4 FrustumProjection::GetProjectionMatrix() const noexcept
{
    return m_projection;
}

float FrustumProjection::GetNearPlaneDistance() const noexcept
{
    return -m_nearMin.z;
}

float FrustumProjection::GetFarPlaneDistance() const noexcept
{
    return -m_farMax.z;
}

AABB FrustumProjection::GetAABB() const noexcept
{
    return m_aabb;
}

std::vector<glm::vec3> FrustumProjection::GetBoundingPoints() const noexcept
{
    return {m_nearMin, m_nearMax, m_farMin, m_farMax};
}

glm::vec3 FrustumProjection::GetNearPlaneMin() const noexcept
{
    return m_nearMin;
}

glm::vec3 FrustumProjection::GetNearPlaneMax() const noexcept
{
    return m_nearMax;
}

glm::vec3 FrustumProjection::GetFarPlaneMin() const noexcept
{
    return m_farMin;
}

glm::vec3 FrustumProjection::GetFarPlaneMax() const noexcept
{
    return m_farMax;
}

bool FrustumProjection::SetNearPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance <= GetFarPlaneDistance());
    if (distance > GetFarPlaneDistance()) { return false; }

    const float leftNear = m_leftTanHalfAngle * distance;
    const float rightNear = m_rightTanHalfAngle * distance;
    const float topNear = m_topTanHalfAngle * distance;
    const float bottomNear = m_bottomTanHalfAngle * distance;

    m_nearMin = glm::vec3(leftNear, bottomNear, -distance);
    m_nearMax = glm::vec3(rightNear, topNear, -distance);

    ComputeAncillary();

    return true;
}

bool FrustumProjection::SetFarPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance >= GetNearPlaneDistance());
    if (distance < GetNearPlaneDistance()) { return false; }

    const float leftFar = m_leftTanHalfAngle * distance;
    const float rightFar = m_rightTanHalfAngle * distance;
    const float topFar = m_topTanHalfAngle * distance;
    const float bottomFar = m_bottomTanHalfAngle * distance;

    m_farMin = glm::vec3(leftFar, bottomFar, -distance);
    m_farMax = glm::vec3(rightFar, topFar, -distance);

    ComputeAncillary();

    return true;
}

void FrustumProjection::ComputeAncillary()
{
    m_projection = glm::frustum(
        m_nearMin.x,
        m_nearMax.x,
        m_nearMin.y,
        m_nearMax.y,
        -m_nearMin.z,
        -m_farMin.z
    );
    m_projection[1][1] *= -1; // Correct for Vulkan's inverted Y-axis

    m_aabb = AABB(GetBoundingPoints());
}

}
