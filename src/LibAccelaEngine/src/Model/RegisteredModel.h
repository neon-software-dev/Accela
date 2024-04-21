/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_SRC_MODEL_REGISTEREDMODEL_H
#define LIBACCELAENGINE_SRC_MODEL_REGISTEREDMODEL_H

#include <Accela/Engine/Model/Model.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <vector>
#include <unordered_map>

namespace Accela::Engine
{
    struct LoadedModelMesh
    {
        Render::MeshId meshId{Render::INVALID_ID};
        Render::MaterialId meshMaterialId{};
    };

    struct RegisteredModel
    {
        // The parsed model definition
        Model::Ptr model;

        // Data that was loaded into the renderer for each mesh in the model
        //
        // model mesh index -> loaded mesh data
        std::unordered_map<unsigned int, LoadedModelMesh> loadedMeshes;

        // The unique textures that were loaded for the model's materials. (Note that
        // textures could theoretically be re-used between meshes, so this is mainly to
        // prevent double resource loads/deletions).
        //
        // texture file name -> loaded texture id
        std::unordered_map<std::string, Render::TextureId> loadedTextures{};
    };
}

#endif //LIBACCELAENGINE_SRC_MODEL_REGISTEREDMODEL_H
