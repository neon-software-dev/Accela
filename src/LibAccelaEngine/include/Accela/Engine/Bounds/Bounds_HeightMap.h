/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_HEIGHTMAP_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_HEIGHTMAP_H

#include <Accela/Render/Id.h>

namespace Accela::Engine
{
    /**
     * Mesh-sourced height-map bounds for a physics object
     */
    struct Bounds_HeightMap
    {
        explicit Bounds_HeightMap(Render::MeshId _heightMapMeshId)
            : heightMapMeshId(_heightMapMeshId)
        { }

        Render::MeshId heightMapMeshId;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_HEIGHTMAP_H
