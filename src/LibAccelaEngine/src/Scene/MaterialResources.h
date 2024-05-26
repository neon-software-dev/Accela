/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_MATERIALRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_MATERIALRESOURCES_H

#include "../ForwardDeclares.h"

#include <Accela/Engine/Scene/IMaterialResources.h>

#include <Accela/Render/Id.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_map>
#include <expected>

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class MaterialResources : public IMaterialResources
    {
        public:

            MaterialResources(Common::ILogger::Ptr logger,
                              ITextureResourcesPtr textures,
                              std::shared_ptr<Render::IRenderer> renderer,
                              std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IMaterialResources
            //
            [[nodiscard]] std::future<Render::MaterialId> LoadObjectMaterial(const CustomResourceIdentifier& resource,
                                                                             const ObjectMaterialProperties& properties,
                                                                             ResultWhen resultWhen) override;
            [[nodiscard]] std::optional<Render::MaterialId> GetMaterialId(const ResourceIdentifier& resource) const override;
            void DestroyMaterial(const ResourceIdentifier& resource) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] Render::MaterialId OnLoadObjectMaterial(
                const CustomResourceIdentifier& resource,
                const ObjectMaterialProperties& properties,
                ResultWhen resultWhen
            );

            [[nodiscard]] std::expected<Render::Material::Ptr, bool> ToRenderMaterial(
                const CustomResourceIdentifier& resource,
                const ObjectMaterialProperties& properties,
                ResultWhen resultWhen) const;

            [[nodiscard]] bool ResolveMaterialTexture(const std::optional<ResourceIdentifier>& resource,
                                                      Render::TextureId& out,
                                                      ResultWhen resultWhen) const;

        private:

            Common::ILogger::Ptr m_logger;
            ITextureResourcesPtr m_textures;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_materialsMutex;
            std::unordered_map<ResourceIdentifier, Render::MaterialId> m_materials;
    };
}


#endif //LIBACCELAENGINE_SRC_SCENE_MATERIALRESOURCES_H
