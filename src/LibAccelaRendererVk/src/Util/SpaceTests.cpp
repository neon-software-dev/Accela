/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SpaceTests.h"

#include <algorithm>
#include <iterator>

namespace Accela::Render
{

bool VolumeTriviallyOutsideProjection(const Volume& volume, const glm::mat4& projection)
{
    const auto boundingPoints = volume.GetBoundingPoints();

    // Outcode is used to keep track of which projectionFrustum planes each bounding point falls outside of
    std::array<uint8_t, 8> outCodes{0};

    for (unsigned int x = 0; x < 8; ++x)
    {
        const auto clipPoint = projection * glm::vec4(boundingPoints[x], 1);

        if (clipPoint.x < -clipPoint.w) { outCodes[x] |= 0x01; }
        if (clipPoint.x > clipPoint.w)  { outCodes[x] |= 0x02; }
        if (clipPoint.y < -clipPoint.w) { outCodes[x] |= 0x04; }
        if (clipPoint.y > clipPoint.w)  { outCodes[x] |= 0x08; }
        if (clipPoint.z < 0)            { outCodes[x] |= 0x10; }
        if (clipPoint.z > clipPoint.w)  { outCodes[x] |= 0x20; }
    }

    // If all bounding points fall outside (one or more of) the same plane, the aabb can be trivially classified
    // as being completely outside the projectionFrustum
    const bool triviallyOutside =
        (   outCodes[0] & outCodes[1] & outCodes[2] & outCodes[3] &
            outCodes[4] & outCodes[5] & outCodes[6] & outCodes[7]
        ) != 0;

    return triviallyOutside;
}

glm::vec3 SlidePointToVolume(const glm::vec3& point, const Volume& volume) noexcept
{
    glm::vec3 slidPoint = point;

    slidPoint.x = std::max(slidPoint.x, volume.min.x);
    slidPoint.x = std::min(slidPoint.x, volume.max.x);

    slidPoint.y = std::max(slidPoint.y, volume.min.y);
    slidPoint.y = std::min(slidPoint.y, volume.max.y);

    slidPoint.z = std::max(slidPoint.z, volume.min.z);
    slidPoint.z = std::min(slidPoint.z, volume.max.z);

    return slidPoint;
}

float DistanceToVolume(const glm::vec3& point, const Volume& volume) noexcept
{
    if (Intersects(point, volume)) { return 0.0f; }

    const auto slidPoint = SlidePointToVolume(point, volume);

    return glm::distance(slidPoint, point);
}

bool Intersects(const Volume& a, const Volume& b) noexcept
{
    if (a.min.x > b.max.x) return false;
    if (a.max.x < b.min.x) return false;
    if (a.min.y > b.max.y) return false;
    if (a.max.y < b.min.y) return false;
    if (a.min.z > b.max.z) return false;
    if (a.max.z < b.min.z) return false;

    return true;
}

bool Intersects(const glm::vec3& point, const Volume& volume) noexcept
{
    return (
        point.x >= volume.min.x && point.x <= volume.max.x &&
        point.y >= volume.min.y && point.y <= volume.max.y &&
        point.z >= volume.min.z && point.z <= volume.max.z
    );
}

bool Intersects(const Sphere& sphere, const Volume& volume) noexcept
{
    return DistanceToVolume(sphere.center, volume) <= sphere.radius;
}

AABB AABBForTransformedProjection(const Projection::Ptr& projection, const glm::mat4& transform)
{
    std::vector<glm::vec3> transformedPoints;

    std::ranges::transform(projection->GetBoundingPoints(), std::back_inserter(transformedPoints),
       [&](const auto& point){
            return transform * glm::vec4(point, 1.0f);
    });

    return AABB(transformedPoints);
}

}
