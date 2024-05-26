/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_MESHVERTEX_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_MESHVERTEX_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    /**
     * A mesh vertex for a StaticMesh
     */
    struct MeshVertex
    {
        MeshVertex(const glm::vec3& _position,
                   const glm::vec3& _normal,
                   const glm::vec2& _uv,
                   const glm::vec3& _tangent = glm::vec3(0))
            : position(_position)
            , normal(_normal)
            , uv(_uv)
            , tangent(_tangent)
        { }

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_MESHVERTEX_H
