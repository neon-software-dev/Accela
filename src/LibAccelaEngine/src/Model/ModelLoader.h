/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_SRC_MODEL_MODELLOADER_H
#define LIBACCELAENGINE_SRC_MODEL_MODELLOADER_H

#include <Accela/Engine/Model/Model.h>
#include <Accela/Engine/Model/ModelNode.h>
#include <Accela/Engine/Model/ModelMaterial.h>
#include <Accela/Engine/Model/ModelMesh.h>
#include <Accela/Engine/Model/ModelBone.h>

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Log/ILogger.h>

#include <assimp/scene.h>

#include <vector>
#include <string>

namespace Accela::Engine
{
    class ModelLoader
    {
        public:

            ModelLoader(Common::ILogger::Ptr logger, Platform::IFiles::Ptr files);

            [[nodiscard]] Model::Ptr LoadModel(const std::string& filePath) const;

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

            std::vector<ModelTexture> GetMaterialTextures(const aiMaterial* pMaterial, aiTextureType type) const;

            static ModelNode::Ptr FindNodeByName(const Model::Ptr& model, const std::string& name);

        private:

            Common::ILogger::Ptr m_logger;
            Platform::IFiles::Ptr m_files;
    };
}

#endif //LIBACCELAENGINE_SRC_MODEL_MODELLOADER_H
