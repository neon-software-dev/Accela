/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_BONEMESHVERTEX_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_BONEMESHVERTEX_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

namespace Accela::Render
{
    /**
     * A mesh vertex for a BoneMesh
     */
    struct ACCELA_PUBLIC BoneMeshVertex
    {
        BoneMeshVertex(const glm::vec3& _position,
                       const glm::vec3& _normal,
                       const glm::vec2& _uv,
                       const glm::vec3& _tangent = glm::vec3(0))
            : position(_position)
            , normal(_normal)
            , uv(_uv)
            , tangent(_tangent)
        { }

        BoneMeshVertex(const glm::vec3& _position,
                       const glm::vec3& _normal,
                       const glm::vec2& _uv,
                       const glm::vec3& _tangent,
                       const glm::ivec4& _bones,
                       const glm::vec4 _boneWeights)
            : position(_position)
            , normal(_normal)
            , uv(_uv)
            , tangent(_tangent)
            , bones(_bones)
            , boneWeights(_boneWeights)
        { }

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;

        glm::ivec4 bones{-1}; // Up to 4 bones that affect the vertex
        glm::vec4 boneWeights{0.0f}; // Strength of each bone's effect on the vertex
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_BONEMESHVERTEX_H
