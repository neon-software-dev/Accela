/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_HEIGHTMAP_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_HEIGHTMAP_H

#include <Accela/Engine/ResourceIdentifier.h>

namespace Accela::Engine
{
    /**
     * Mesh-sourced height-map bounds for a physics object
     */
    struct Bounds_HeightMap
    {
        /**
         * @param _resource Identifies the mesh resource to use as height map bounds
         */
        explicit Bounds_HeightMap(ResourceIdentifier _resource)
            : resource(std::move(_resource))
        { }

        ResourceIdentifier resource;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_HEIGHTMAP_H
