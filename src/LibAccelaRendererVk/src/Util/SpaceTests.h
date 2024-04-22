/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_SPACETESTS_H
#define LIBACCELARENDERERVK_SRC_UTIL_SPACETESTS_H

#include "AABB.h"
#include "Volume.h"
#include "Projection.h"
#include "Sphere.h"

#include <glm/glm.hpp>

#include <algorithm>

namespace Accela::Render
{

    /**
     * Returns whether an AABB is trivially outside the bounds of a projectionFrustum.
     *
     * Note that this should only be used for imperfect culling logic; it will only say that the AABB
     * is outside of the projectionFrustum if it is *TRIVIALLY* outside of the projectionFrustum. More complex cases where
     * the AABB is outside multiple planes of the projectionFrustum, will err on the side of caution and will be
     * reported as not trivially outside the projectionFrustum, even if in actuality the AABB might not be visible
     * within the projectionFrustum.
     *
     * @param aabb The AABB to be transformed
     * @param projection The projectionFrustum transformation to apply
     *
     * @return Whether the AABB is trivially outside of the projectionFrustum.
     */
    [[nodiscard]] bool VolumeTriviallyOutsideProjection(const Volume& volume, const glm::mat4& projection);

    /**
     * Returns the point on the surface of the volume which is closest to the provided point.
     *
     * Warning! If the provided point is within the volume's bounds, the point itself will be returned.
     */
    [[nodiscard]] glm::vec3 SlidePointToVolume(const glm::vec3& point, const Volume& volume) noexcept;

    /**
     * @return The minimum distance between the provided point and volume (0.0f if the point is within
     * the volume already).
     */
    [[nodiscard]] float DistanceToVolume(const glm::vec3& point, const Volume& volume) noexcept;

    [[nodiscard]] bool Intersects(const Volume& a, const Volume& b) noexcept;
    [[nodiscard]] bool Intersects(const glm::vec3& point, const Volume& volume) noexcept;
    [[nodiscard]] bool Intersects(const Sphere& sphere, const Volume& volume) noexcept;

    /**
     * Applies the provided transform to the provided projection's bounding points, then returns a new AABB
     * from the transformed points.
     */
    [[nodiscard]] AABB AABBForTransformedProjection(const Projection::Ptr& projection, const glm::mat4& transform);
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_SPACETESTS_H
