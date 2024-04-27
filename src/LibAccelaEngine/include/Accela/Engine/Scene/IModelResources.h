/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMODELRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMODELRESOURCES_H

#include <Accela/Engine/Common.h>

#include <Accela/Engine/Model/Model.h>

#include <memory>
#include <string>
#include <future>

namespace Accela::Engine
{
    class IModelResources
    {
        public:

            using Ptr = std::shared_ptr<IModelResources>;

        public:

            virtual ~IModelResources() = default;

            /**
             * Loads a model for rendering
             *
             * @param modelName A unique name to identify the model
             * @param model The model's data
             * @param resultWhen When the operation's future should be signaled
             *
             * @return A future for the result of the operation
             */
            [[nodiscard]] virtual std::future<bool> LoadModel(const std::string& modelName,
                                                              const Model::Ptr& model,
                                                              ResultWhen resultWhen) = 0;

            /**
             * Destroy a previously loaded model
             *
             * @param modelName The unique name identifying the model
             */
            virtual void DestroyModel(const std::string& modelName) = 0;

            /**
             * Destroy all previously loaded models
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMODELRESOURCES_H
