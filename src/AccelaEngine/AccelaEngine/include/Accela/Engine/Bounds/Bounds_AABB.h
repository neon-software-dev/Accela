/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_AABB_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_AABB_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

namespace Accela::Engine
{
    /**
     * AABB bounds for a physics object
     */
    struct ACCELA_PUBLIC Bounds_AABB
    {
        Bounds_AABB(const glm::vec3& _min,
                    const glm::vec3& _max)
            : min(_min)
            , max(_max)
        { }

        glm::vec3 min;
        glm::vec3 max;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_AABB_H
