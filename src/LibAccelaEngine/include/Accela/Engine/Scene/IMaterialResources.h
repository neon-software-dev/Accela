/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMATERIALRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMATERIALRESOURCES_H

#include <Accela/Engine/Common.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <memory>
#include <future>

namespace Accela::Engine
{
    class IMaterialResources
    {
        public:

            using Ptr = std::shared_ptr<IMaterialResources>;

        public:

            virtual ~IMaterialResources() = default;


            /**
             * Load an object material
             *
             * @param properties The properties of the object material
             * @param tag A debug tag to associate with the material
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::MaterialId> LoadObjectMaterial(
                const Render::ObjectMaterialProperties& properties,
                const std::string& tag,
                ResultWhen resultWhen
            ) = 0;

            /**
             * Destroy a previously registered material
             *
             * @param materialId The MaterialId of the material to be destroyed
             */
            virtual void DestroyMaterial(Render::MaterialId materialId) = 0;

            /**
             * Destroy all previously loaded materials
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMATERIALRESOURCES_H
