/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_CAPSULE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_CAPSULE_H

#include <Accela/Common/SharedLib.h>

namespace Accela::Engine
{
    /**
     * Capsule bounds for a physics object
     */
    struct ACCELA_PUBLIC Bounds_Capsule
    {
        Bounds_Capsule(const float& _radius, const float& _height)
            : radius(_radius)
            , height(_height)
        { }

        float radius;
        float height;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_CAPSULE_H
