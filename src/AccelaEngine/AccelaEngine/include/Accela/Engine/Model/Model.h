/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODEL_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODEL_H

#include "ModelNode.h"
#include "ModelMesh.h"
#include "ModelBone.h"
#include "ModelAnimation.h"

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Accela::Engine
{
    /**
     * Engine representation of a model. Can be created programmatically or loaded
     * from Assets via IEngineAssets::ReadModelBlocking.
     */
    struct ACCELA_PUBLIC Model
    {
        using Ptr = std::shared_ptr<Model>;

        //
        // Node Data
        //
            ModelNode::Ptr rootNode;

            // model node id -> node data
            std::unordered_map<unsigned int, ModelNode::Ptr> nodeMap;

            // model node ids
            std::unordered_set<unsigned int> nodesWithMeshes;

        //
        // Mesh/Material Data
        //

            // mesh index -> mesh data
            std::unordered_map<unsigned int, ModelMesh> meshes;

            // material index -> material data
            std::unordered_map<unsigned int, ModelMaterial> materials;

        //
        // Animation Data
        //

            // animation name -> animation data
            std::unordered_map<std::string, ModelAnimation> animations;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODEL_H
