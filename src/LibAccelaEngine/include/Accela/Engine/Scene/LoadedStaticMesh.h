/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_LOADEDSTATICMESH_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_LOADEDSTATICMESH_H

#include <Accela/Render/Mesh/MeshVertex.h>

#include <vector>
#include <cstdint>
#include <memory>

namespace Accela::Engine
{
    struct LoadedStaticMesh
    {
        using Ptr = std::shared_ptr<LoadedStaticMesh>;

        LoadedStaticMesh(std::vector<Render::MeshVertex> _vertices, std::vector<uint32_t> _indices)
            : vertices(std::move(_vertices))
            , indices(std::move(_indices))
        { }

        std::vector<Render::MeshVertex> vertices;
        std::vector<uint32_t> indices;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_LOADEDSTATICMESH_H
