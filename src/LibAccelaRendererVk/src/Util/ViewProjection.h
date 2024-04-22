/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_VIEWPROJECTION_H
#define LIBACCELARENDERERVK_SRC_UTIL_VIEWPROJECTION_H

#include "Projection.h"
#include "AABB.h"
#include "SpaceTests.h"

#include <glm/glm.hpp>

namespace Accela::Render
{
    struct ViewProjection
    {
        ViewProjection(const glm::mat4& _viewTransform, Projection::Ptr _projectionTransform)
            : viewTransform(_viewTransform)
            , projectionTransform(std::move(_projectionTransform))
        { }

        [[nodiscard]] glm::mat4 GetTransformation() const
        {
            return projectionTransform->GetProjectionMatrix() * viewTransform;
        }

        /**
         * Positions the projection's bounds in world space via the view transform and
         * computes an AABB from the world-spaced points. Returns a bounding box of what
         * portion of world-space the ViewProjection covers.
         */
        [[nodiscard]] AABB GetWorldSpaceAABB() const
        {
            return AABBForTransformedProjection(
                projectionTransform,
                glm::inverse(viewTransform)
            );
        }

        glm::mat4 viewTransform{1};
        Projection::Ptr projectionTransform;
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_VIEWPROJECTION_H
