#include "OrthoProjection.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Accela::Render
{

std::expected<Projection::Ptr, bool> OrthoProjection::From(const glm::vec3& nearMin,
                                                           const glm::vec3& nearMax,
                                                           const glm::vec3& farMin,
                                                           const glm::vec3& farMax)
{
    // Points must be on x/y plane
    if (nearMin.z != nearMax.z) { return std::unexpected(false); }
    if (farMin.z != farMax.z) { return std::unexpected(false); }

    // Points must be rectangular
    if (nearMin.x != farMin.x) { return std::unexpected(false); }
    if (nearMax.x != farMax.x) { return std::unexpected(false); }
    if (nearMin.y != farMin.y) { return std::unexpected(false); }
    if (nearMax.y != farMax.y) { return std::unexpected(false); }

    // Near points must be closer than far points
    if (nearMin.z <= farMin.z) { return std::unexpected(false); }
    if (nearMax.z <= farMax.z) { return std::unexpected(false); }

    return std::make_shared<OrthoProjection>(Tag{}, nearMin, nearMax, farMin, farMax);
}

std::expected<Projection::Ptr, bool> OrthoProjection::From(const float& width,
                                                           const float& height,
                                                           const float& nearDistance,
                                                           const float& farDistance)
{
    // Dimensions must be non-zero positive
    if (width <= 0.0f) { return std::unexpected(false); }
    if (height <= 0.0f) { return std::unexpected(false); }
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    const float halfWidth = width / 2.0f;
    const float halfHeight = height / 2.0f;

    return From(
        glm::vec3(-halfWidth,-halfHeight,-nearDistance),
        glm::vec3(halfWidth,halfHeight,-nearDistance),
        glm::vec3(-halfWidth,-halfHeight,-farDistance),
        glm::vec3(halfWidth,halfHeight,-farDistance)
    );
}

Projection::Ptr OrthoProjection::Clone() const
{
    return std::make_shared<OrthoProjection>(Tag{}, m_nearMin, m_nearMax, m_farMin, m_farMax);
}

OrthoProjection::OrthoProjection(Tag,
                                 const glm::vec3& nearMin,
                                 const glm::vec3& nearMax,
                                 const glm::vec3& farMin,
                                 const glm::vec3& farMax)
    : m_nearMin(nearMin)
    , m_nearMax(nearMax)
    , m_farMin(farMin)
    , m_farMax(farMax)
{
    ComputeAncillary();
}

glm::mat4 OrthoProjection::GetProjectionMatrix() const noexcept
{
    return m_projection;
}

float OrthoProjection::GetNearPlaneDistance() const noexcept
{
    return -m_nearMin.z;
}

float OrthoProjection::GetFarPlaneDistance() const noexcept
{
    return -m_farMax.z;
}

AABB OrthoProjection::GetAABB() const noexcept
{
    return m_aabb;
}

std::vector<glm::vec3> OrthoProjection::GetBoundingPoints() const
{
    return {m_nearMin, m_nearMax, m_farMin, m_farMax};
}

glm::vec3 OrthoProjection::GetNearPlaneMin() const noexcept
{
    return m_nearMin;
}

glm::vec3 OrthoProjection::GetNearPlaneMax() const noexcept
{
    return m_nearMax;
}

glm::vec3 OrthoProjection::GetFarPlaneMin() const noexcept
{
    return m_farMin;
}

glm::vec3 OrthoProjection::GetFarPlaneMax() const noexcept
{
    return m_farMax;
}

bool OrthoProjection::SetNearPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance <= GetFarPlaneDistance());
    if (distance > GetFarPlaneDistance()) { return false; }

    m_nearMin.z = -distance;
    m_nearMax.z = -distance;

    return true;
}

bool OrthoProjection::SetFarPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance >= GetNearPlaneDistance());
    if (distance < GetNearPlaneDistance()) { return false; }

    m_farMin.z = -distance;
    m_farMax.z = -distance;

    return true;
}

void OrthoProjection::ComputeAncillary()
{
    m_projection = glm::ortho(
        m_nearMin.x,
        m_nearMax.x,
        m_nearMax.y,
        m_nearMin.y,
        -m_nearMin.z,
        -m_farMax.z
    );

    m_aabb = AABB(GetBoundingPoints());
}

}
