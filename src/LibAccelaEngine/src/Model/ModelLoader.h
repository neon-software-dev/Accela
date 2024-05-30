/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_MODEL_MODELLOADER_H
#define LIBACCELAENGINE_SRC_MODEL_MODELLOADER_H

#include <Accela/Engine/Model/Model.h>
#include <Accela/Engine/Model/ModelNode.h>
#include <Accela/Engine/Model/ModelMaterial.h>
#include <Accela/Engine/Model/ModelMesh.h>
#include <Accela/Engine/Model/ModelBone.h>

#include <Accela/Common/Log/ILogger.h>

#include <assimp/scene.h>

#include <vector>
#include <string>
#include <cstddef>

namespace Accela::Engine
{
    class ModelLoader
    {
        public:

            explicit ModelLoader(Common::ILogger::Ptr logger);

            [[nodiscard]] Model::Ptr LoadModel(const std::vector<std::byte>& modelData,
                                               const std::string& fileHint,
                                               const std::string& tag) const;

        private:

            void ProcessMaterials(const Model::Ptr& model, const aiScene* pScene) const;
            ModelMaterial ProcessMaterial(const aiMaterial* pMaterial) const;

            void ProcessMeshes(const Model::Ptr& model, const aiScene* pScene) const;
            ModelMesh ProcessMesh(const aiMesh* pMesh, const unsigned int& meshIndex) const;
            static ModelMesh ProcessStaticMesh(const aiMesh* pMesh, const unsigned int& meshIndex);
            ModelMesh ProcessBoneMesh(const aiMesh* pMesh, const unsigned int& meshIndex) const;

            void ProcessNodes(const Model::Ptr& model, const aiScene* pScene) const;
            static ModelNode::Ptr ProcessNode(const Model::Ptr& model, const aiNode* pNode);

            static void ProcessSkeletons(const Model::Ptr& model);
            static void ProcessAnimations(const Model::Ptr& model, const aiScene *pScene);

            static ModelAnimation ProcessAnimation(const aiAnimation* pAnimation);

            static void ProcessEmbeddedTextures(const Model::Ptr& model, const aiScene* pScene);
            static void ProcessEmbeddedTexture(const ModelMaterial& material, const aiTexture* pAiTexture, ModelTexture& modelTexture);

            std::vector<ModelTexture> GetMaterialTextures(const aiMaterial* pMaterial, aiTextureType type) const;

            static ModelNode::Ptr FindNodeByName(const Model::Ptr& model, const std::string& name);

        private:

            Common::ILogger::Ptr m_logger;
    };
}

#endif //LIBACCELAENGINE_SRC_MODEL_MODELLOADER_H
