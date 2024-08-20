/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMESH_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMESH_H

#include "ModelMaterial.h"
#include "ModelBone.h"

#include <Accela/Render/Mesh/Mesh.h>
#include <Accela/Render/Mesh/MeshVertex.h>
#include <Accela/Render/Mesh/BoneMeshVertex.h>

#include <Accela/Common/SharedLib.h>

#include <string>
#include <optional>
#include <unordered_map>

namespace Accela::Engine
{
    /**
     * Properties of a specific mesh that a model uses
     */
    struct ACCELA_PUBLIC ModelMesh
    {
        unsigned int meshIndex;
        std::string name;
        Render::MeshType meshType;
        std::optional<std::vector<Render::MeshVertex>> staticVertices;
        std::optional<std::vector<Render::BoneMeshVertex>> boneVertices;
        std::vector<uint32_t> indices;
        unsigned int materialIndex{0};

        // Bone name -> bone info
        std::unordered_map<std::string, ModelBone> boneMap;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMESH_H
