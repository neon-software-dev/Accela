/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_STATICMESH_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_STATICMESH_H

#include "Mesh.h"
#include "MeshVertex.h"

namespace Accela::Render
{
    /**
     * A simple/static mesh with no skeleton
     */
    struct StaticMesh : public Mesh
    {
        StaticMesh(MeshId meshId, std::string _tag)
            : Mesh(MeshType::Static, meshId, std::move(_tag))
        { }

        StaticMesh(MeshId meshId, std::vector<MeshVertex> _vertices, std::vector<uint32_t> _indices, std::string _tag)
            : Mesh(MeshType::Static, meshId, std::move(_tag))
            , vertices(std::move(_vertices))
            , indices(std::move(_indices))
        { }

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_STATICMESH_H
