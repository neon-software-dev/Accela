/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_RAYCASTRESULT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_RAYCASTRESULT_H

#include <Accela/Engine/Common.h>

#include <glm/glm.hpp>

namespace Accela::Engine
{
    /**
     * Holds the details of a raycast hit
     */
    struct RaycastResult
    {
        RaycastResult(EntityId _eid, const glm::vec3& _hitPoint_worldSpace, const glm::vec3& _hitNormal_worldSpace)
            : eid(_eid)
            , hitPoint_worldSpace(_hitPoint_worldSpace)
            , hitNormal_worldSpace(_hitNormal_worldSpace)
        { }

        EntityId eid;                   // EntityId associated with the body that was hit
        glm::vec3 hitPoint_worldSpace;  // The world-space coordinate of the geometry that was hit
        glm::vec3 hitNormal_worldSpace; // The world-space normal of the geometry that was hit
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_RAYCASTRESULT_H
