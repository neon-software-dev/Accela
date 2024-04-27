/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_MATERIALRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_MATERIALRESOURCES_H

#include <Accela/Engine/Scene/IMaterialResources.h>

#include <Accela/Render/IRenderer.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_set>

namespace Accela::Engine
{
    class MaterialResources : public IMaterialResources
    {
        public:

            MaterialResources(Common::ILogger::Ptr logger,
                              std::shared_ptr<Render::IRenderer> renderer,
                              std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IMaterialResources
            //
            [[nodiscard]] std::future<Render::MaterialId> LoadObjectMaterial(
                const Render::ObjectMaterialProperties& properties,
                const std::string& tag,
                ResultWhen resultWhen
            ) override;

            void DestroyMaterial(Render::MaterialId materialId) override;

            void DestroyAll() override;

        private:

            [[nodiscard]] Render::MaterialId OnLoadObjectMaterial(
                const Render::ObjectMaterialProperties& properties,
                const std::string& tag,
                ResultWhen resultWhen
            );

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_materialsMutex;
            std::unordered_set<Render::MaterialId> m_materials; // Ids of loaded materials
    };
}


#endif //LIBACCELAENGINE_SRC_SCENE_MATERIALRESOURCES_H
