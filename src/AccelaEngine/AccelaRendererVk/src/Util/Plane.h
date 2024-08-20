/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_PLANE_H
#define LIBACCELARENDERERVK_SRC_UTIL_PLANE_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    struct Plane
    {
        Plane() = default;
        
        Plane(const glm::vec3& _point, const glm::vec3& _normalUnit)
            : point(_point)
            , normalUnit(_normalUnit)
        { }

        glm::vec3 point{0};         // A point on the plane
        glm::vec3 normalUnit{0};    // Unit vector of the plane's normal
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_PLANE_H
