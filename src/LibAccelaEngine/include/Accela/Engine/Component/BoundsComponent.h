/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_BOUNDSCOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_BOUNDSCOMPONENT_H

#include <Accela/Engine/Bounds/Bounds_AABB.h>
#include <Accela/Engine/Bounds/Bounds_Sphere.h>
#include <Accela/Engine/Bounds/Bounds_Capsule.h>
#include <Accela/Engine/Bounds/Bounds_HeightMap.h>

#include <variant>

namespace Accela::Engine
{
    /**
     * Attaches to an entity to describe the bounds of the entity, for physics purposes
     */
    struct BoundsComponent
    {
        using BoundsVariant = std::variant<
            Bounds_AABB,
            Bounds_Sphere,
            Bounds_Capsule,
            Bounds_HeightMap
        >;

        explicit BoundsComponent(const BoundsVariant& _bounds,
                                 const glm::vec3& _localTransform = glm::vec3(0),
                                 const glm::quat& _localOrientation = glm::identity<glm::quat>())
            : bounds(_bounds)
            , localTransform(_localTransform)
            , localOrientation(_localOrientation)
        { }

        BoundsVariant bounds; // Model-space bounds
        glm::vec3 localTransform; // Local transform of the bounds relative to entity's model space
        glm::quat localOrientation; // Local orientation of the bounds relative to the entity's model space
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_BOUNDSCOMPONENT_H
