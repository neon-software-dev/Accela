/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMODELRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMODELRESOURCES_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <Accela/Engine/Model/Model.h>

#include <Accela/Common/ImageData.h>

#include <memory>
#include <string>
#include <future>
#include <unordered_map>

namespace Accela::Engine
{
    // Texture FileName -> Image data (for non-embedded textures)
    using ModelTextures = std::unordered_map<std::string, Common::ImageData::Ptr>;

    class IModelResources
    {
        public:

            using Ptr = std::shared_ptr<IModelResources>;

        public:

            virtual ~IModelResources() = default;

            /**
             * Loads a model resource from the specified package
             *
             * @param resource Identifies the model resource
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing whether the load was successful
             */
            [[nodiscard]] virtual std::future<bool> LoadModel(const PackageResourceIdentifier& resource, ResultWhen resultWhen) = 0;

            /**
             * Load all model resources from the specified package
             *
             * @param packageName Identifies the package
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing whether all model loads were successful
             */
            [[nodiscard]] virtual std::future<bool> LoadAllModels(const PackageName& packageName, ResultWhen resultWhen) = 0;

            /**
             * Load all model resources from all registered packages
             *
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing whether all model loads were successful
             */
            [[nodiscard]] virtual std::future<bool> LoadAllModels(ResultWhen resultWhen) = 0;

            /**
             * Loads a custom model resource
             *
             * @param resource Identifies the model resource
             * @param model The model's data
             * @param modelTextures Texture data for the model's non-embedded textures (texture filename -> data)
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing whether the model load was successful
             */
            [[nodiscard]] virtual std::future<bool> LoadModel(const CustomResourceIdentifier& resource,
                                                              const Model::Ptr& model,
                                                              const ModelTextures& modelTextures,
                                                              ResultWhen resultWhen) = 0;

            /**
             * Destroy a previously loaded model resource
             *
             * @param resource Identifies the model resource
             */
            virtual void DestroyModel(const ResourceIdentifier& resource) = 0;

            /**
             * Destroy all previously loaded model resources
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMODELRESOURCES_H
