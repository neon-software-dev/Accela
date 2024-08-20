/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMATERIALRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMATERIALRESOURCES_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/ResourceIdentifier.h>
#include <Accela/Engine/Material/ObjectMaterialProperties.h>

#include <Accela/Render/Id.h>

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <future>
#include <optional>

namespace Accela::Engine
{
    /**
     * Encapsulates material resource operations
     */
    class ACCELA_PUBLIC IMaterialResources
    {
        public:

            using Ptr = std::shared_ptr<IMaterialResources>;

        public:

            virtual ~IMaterialResources() = default;

            /**
             * Load a custom object material resource
             *
             * @param resource Identifies the material resource
             * @param properties The properties of the object material
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing the loaded materialId, or INVALID_ID on error
             */
            [[nodiscard]] virtual std::future<Render::MaterialId> LoadObjectMaterial(
                const CustomResourceIdentifier& resource,
                const ObjectMaterialProperties& properties,
                ResultWhen resultWhen
            ) = 0;

            /**
             * Returns the MaterialId associated with a previously loaded material resource
             *
             * @param resource Identifies the material resource
             *
             * @return The loaded MaterialId, or std::nullopt if no such material
             */
            [[nodiscard]] virtual std::optional<Render::MaterialId> GetMaterialId(const ResourceIdentifier& resource) const = 0;

            /**
             * Destroy a previously registered material
             *
             * @param resource Identifies the material to be destroyed
             */
            virtual void DestroyMaterial(const ResourceIdentifier& resource) = 0;

            /**
             * Destroy all previously loaded materials
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMATERIALRESOURCES_H
