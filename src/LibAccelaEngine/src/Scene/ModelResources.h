/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_MODELRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_MODELRESOURCES_H

#include "../ForwardDeclares.h"

#include "../Model/RegisteredModel.h"
#include "../Model/ModelLoader.h"

#include <Accela/Engine/Scene/IModelResources.h>

#include "Accela/Platform/Package/PackageSource.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_map>
#include <expected>
#include <vector>

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Platform
{
    class IFiles;
}

namespace Accela::Engine
{
    class ModelResources : public IModelResources
    {
        public:

            ModelResources(Common::ILogger::Ptr logger,
                           IPackageResourcesPtr packages,
                           std::shared_ptr<Render::IRenderer> renderer,
                           std::shared_ptr<Platform::IFiles> files,
                           std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IModelResources
            //
            [[nodiscard]] std::future<bool> LoadModel(const PackageResourceIdentifier& resource, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<bool> LoadAllModels(const PackageName& packageName, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<bool> LoadAllModels(ResultWhen resultWhen) override;
            [[nodiscard]] std::future<bool> LoadModel(const CustomResourceIdentifier& resource,
                                                      const Model::Ptr& model,
                                                      const ModelTextures& modelTextures,
                                                      ResultWhen resultWhen) override;
            void DestroyModel(const ResourceIdentifier& resource) override;
            void DestroyAll() override;

            //
            // Internal
            //
            [[nodiscard]] std::optional<RegisteredModel> GetLoadedModel(const ResourceIdentifier& resource) const;

        private:

            [[nodiscard]] bool OnLoadModel(const PackageResourceIdentifier& resource, ResultWhen resultWhen);
            [[nodiscard]] bool OnLoadAllModels(const PackageName& packageName, ResultWhen resultWhen);
            [[nodiscard]] bool OnLoadAllModels(ResultWhen resultWhen);

            [[nodiscard]] bool LoadPackageModelInternal(const ResourceIdentifier& resource,
                                                        const Model::Ptr& model,
                                                        const ModelTextures& modelTextures,
                                                        ResultWhen resultWhen);

            [[nodiscard]] std::expected<ModelTextures, bool> LoadPackageModelTextures(
                const PackageResourceIdentifier& resource,
                const Model::Ptr& model,
                const Platform::PackageSource::Ptr& package);

            [[nodiscard]] bool LoadPackageModelTextures(
                const PackageResourceIdentifier& resource,
                const std::vector<ModelTexture>& textures,
                const Platform::PackageSource::Ptr& package,
                ModelTextures& result);

            [[nodiscard]] std::expected<Render::MaterialId, bool> LoadModelMeshMaterial(
                RegisteredModel& registeredModel,
                const std::string& modelName,
                const ModelMaterial& material,
                const std::unordered_map<std::string, Common::ImageData::Ptr>& modelTextures,
                ResultWhen resultWhen) const;

            [[nodiscard]] std::expected<Render::TextureId, bool> LoadModelMaterialTexture(
                RegisteredModel& registeredModel,
                const std::string& modelName,
                const ModelTexture& modelTexture,
                const std::unordered_map<std::string, Common::ImageData::Ptr>& modelTextures,
                ResultWhen resultWhen) const;

            [[nodiscard]] std::expected<Render::MeshId, bool> LoadModelMesh(
                RegisteredModel& registeredModel,
                const std::unordered_map<unsigned int, Render::MaterialId>& registeredMaterials,
                const ModelMesh& modelMesh,
                ResultWhen resultWhen) const;

        private:

            Common::ILogger::Ptr m_logger;
            IPackageResourcesPtr m_packages;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;
            ModelLoader m_modelLoader;

            mutable std::recursive_mutex m_modelsMutex;
            std::unordered_map<ResourceIdentifier, RegisteredModel> m_models;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_MODELRESOURCES_H
