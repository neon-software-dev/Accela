/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_WORLDRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_WORLDRESOURCES_H

#include "../ForwardDeclares.h"

#include "../Model/RegisteredModel.h"

#include <Accela/Engine/Scene/IWorldResources.h>

#include <Accela/Render/Texture/Texture.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <expected>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Platform
{
    class IFiles;
    class IText;
}

namespace Accela::Engine
{
    class WorldResources : public IWorldResources
    {
        public:

            WorldResources(Common::ILogger::Ptr logger,
                           std::shared_ptr<Render::IRenderer> renderer,
                           std::shared_ptr<Platform::IFiles> files,
                           std::shared_ptr<Platform::IText> text,
                           AudioManagerPtr audioManager);

            [[nodiscard]] IPackageResources::Ptr Packages() const override;
            [[nodiscard]] ITextureResources::Ptr Textures() const override;
            [[nodiscard]] IMeshResources::Ptr Meshes() const override;
            [[nodiscard]] IMaterialResources::Ptr Materials() const override;
            [[nodiscard]] IAudioResources::Ptr Audio() const override;
            [[nodiscard]] IFontResources::Ptr Fonts() const override;
            [[nodiscard]] IModelResources::Ptr Models() const override;

            [[nodiscard]] std::future<bool> EnsurePackageResources(const PackageName& packageName, ResultWhen resultWhen) override;

            void DestroyAll() override;

        private:

            [[nodiscard]] bool OnEnsurePackageResources(const PackageName& packageName, ResultWhen resultWhen) const;

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Platform::IText> m_text;
            AudioManagerPtr m_audioManager;

            IPackageResources::Ptr m_packages;
            ITextureResources::Ptr m_textures;
            IMeshResources::Ptr m_meshes;
            IMaterialResources::Ptr m_materials;
            IAudioResources::Ptr m_audio;
            IFontResources::Ptr m_fonts;
            IModelResources::Ptr m_models;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_WORLDRESOURCES_H
