/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_MODELRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_MODELRESOURCES_H

#include "../Model/RegisteredModel.h"

#include <Accela/Engine/Scene/IModelResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_map>
#include <expected>
#include <vector>
#include <utility>

namespace Accela::Platform
{
    class IFiles;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class IEngineAssets;

    class ModelResources : public IModelResources
    {
        public:

            ModelResources(Common::ILogger::Ptr logger,
                           std::shared_ptr<Render::IRenderer> renderer,
                           std::shared_ptr<IEngineAssets> assets,
                           std::shared_ptr<Platform::IFiles> files,
                           std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IModelResources
            //
            [[nodiscard]] std::future<bool> LoadAssetsModel(const std::string& modelFileName, ResultWhen resultWhen) override;

            [[nodiscard]] std::future<bool> LoadAllAssetModels(ResultWhen resultWhen) override;

            [[nodiscard]] std::future<bool> LoadModel(const std::string& modelName,
                                                      const Model::Ptr& model,
                                                      ResultWhen resultWhen) override;

            void DestroyModel(const std::string& modelName) override;

            void DestroyAll() override;

            //
            // Internal
            //
            [[nodiscard]] std::optional<RegisteredModel> GetLoadedModel(const std::string& modelName) const;

        private:

            [[nodiscard]] bool OnLoadAssetsModel(const std::string& modelFileName, ResultWhen resultWhen);

            [[nodiscard]] bool OnLoadAllAssetModels(ResultWhen resultWhen);
            [[nodiscard]] std::vector<std::string> GetAllAssetModelFileNames() const;

            [[nodiscard]] bool OnLoadModel(const std::string& modelName,
                                           const Model::Ptr& model,
                                           ResultWhen resultWhen);

            [[nodiscard]] std::expected<Render::MaterialId, bool> LoadModelMeshMaterial(
                RegisteredModel& registeredModel,
                const std::string& modelName,
                const ModelMaterial& material,
                ResultWhen resultWhen) const;

            [[nodiscard]] std::expected<Render::TextureId, bool> LoadModelMaterialTexture(
                RegisteredModel& registeredModel,
                const std::string& modelName,
                const ModelTexture& modelTexture,
                ResultWhen resultWhen) const;

            [[nodiscard]] std::expected<Render::MeshId, bool> LoadModelMesh(
                RegisteredModel& registeredModel,
                const std::unordered_map<unsigned int, Render::MaterialId>& registeredMaterials,
                const ModelMesh& modelMesh,
                ResultWhen resultWhen) const;

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::recursive_mutex m_modelsMutex;
            std::unordered_map<std::string, RegisteredModel> m_models;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_MODELRESOURCES_H
