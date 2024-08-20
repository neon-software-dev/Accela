/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <Accela/Common/SharedLib.h>

#include <optional>

namespace Accela::Engine
{
    /**
     * Static-Mesh-sourced bounds for a physics object
     */
    struct ACCELA_PUBLIC Bounds_StaticMesh
    {
        struct MeshSlice
        {
            std::size_t verticesStartIndex;
            std::size_t verticesCount{0};
            std::size_t indicesStartIndex{0};
            std::size_t indicesCount{0};
        };

        Bounds_StaticMesh(ResourceIdentifier _resource, bool _meshCanContainDuplicateVertices)
            : resource(std::move(_resource))
            , meshCanContainDuplicateVertices(_meshCanContainDuplicateVertices)
        { }

        Bounds_StaticMesh(ResourceIdentifier _resource, bool _meshCanContainDuplicateVertices, MeshSlice _meshSlice)
            : resource(std::move(_resource))
            , meshCanContainDuplicateVertices(_meshCanContainDuplicateVertices)
            , meshSlice(std::move(_meshSlice))
        { }

        ResourceIdentifier resource;

        // Physics system needs to weld duplicate vertices together to play nice with PhysX, so
        // this variable instructs PhysX to run this process. Set it to true if unsure.
        bool meshCanContainDuplicateVertices{false};

        // Set this variable to generate bounds from a specific slice/subset
        // of the mesh resource rather than the entire mesh resource
        std::optional<MeshSlice> meshSlice;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H
