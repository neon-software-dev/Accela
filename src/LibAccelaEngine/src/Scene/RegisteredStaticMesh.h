/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_REGISTEREDSTATICMESH_H
#define LIBACCELAENGINE_SRC_SCENE_REGISTEREDSTATICMESH_H

#include <Accela/Render/Mesh/MeshVertex.h>

#include <vector>
#include <cstdint>
#include <memory>

namespace Accela::Engine
{
    struct RegisteredStaticMesh
    {
        using Ptr = std::shared_ptr<RegisteredStaticMesh>;

        RegisteredStaticMesh(std::vector<Render::MeshVertex> _vertices, std::vector<uint32_t> _indices)
            : vertices(std::move(_vertices))
            , indices(std::move(_indices))
        { }

        std::vector<Render::MeshVertex> vertices;
        std::vector<uint32_t> indices;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_REGISTEREDSTATICMESH_H
