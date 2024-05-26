/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_MESH_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_MESH_H

#include "../Id.h"

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace Accela::Render
{
    enum class MeshUsage
    {
        Static,     // Updated infrequently
        Dynamic,    // Updated frequently
        Immutable   // Updated never
    };

    enum class MeshType
    {
        Static, // Not skeleton-based
        Bone    // Skeleton-based
    };

    /**
     * Base class for a mesh that can be registered with the renderer
     */
    struct Mesh
    {
        using Ptr = std::shared_ptr<Mesh>;

        Mesh(MeshType _type, MeshId _id, std::string _tag)
             : type(_type)
             , id(_id)
             , tag(std::move(_tag))
        { }

        virtual ~Mesh() = default;

        MeshType type;
        MeshId id;
        std::string tag;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_MESH_H
