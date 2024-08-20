/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_SPHERE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_SPHERE_H

#include <Accela/Common/SharedLib.h>

namespace Accela::Engine
{
    /**
     * Spherical bounds for a physics object
     */
    struct ACCELA_PUBLIC Bounds_Sphere
    {
        explicit Bounds_Sphere(const float& _radius)
            : radius(_radius)
        { }

        float radius;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_SPHERE_H
