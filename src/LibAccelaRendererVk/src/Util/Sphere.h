/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_SPHERE_H
#define LIBACCELARENDERERVK_SRC_UTIL_SPHERE_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    struct Sphere
    {
        Sphere() = default;

        Sphere(const glm::vec3& _center, float _radius)
            : center(_center)
            , radius(_radius)
        { }

        glm::vec3 center{0};    // The center point of the sphere
        float radius{0.0f};         // The radius of the sphere
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_SPHERE_H