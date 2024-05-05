/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H

#include <Accela/Render/Id.h>

namespace Accela::Engine
{
    /**
     * Static-Mesh-sourced bounds for a physics object
     */
    struct Bounds_StaticMesh
    {
        explicit Bounds_StaticMesh(Render::MeshId _staticMeshId)
            : staticMeshId(_staticMeshId)
        { }

        Render::MeshId staticMeshId;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H
