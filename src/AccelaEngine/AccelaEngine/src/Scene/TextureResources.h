/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H

#include "../ForwardDeclares.h"
#include "../Texture/RegisteredTexture.h"

#include <Accela/Engine/Scene/ITextureResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <expected>
#include <memory>
#include <future>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <sstream>

namespace Accela::Platform
{
    class IText;
    class IFiles;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class TextureResources : public ITextureResources
    {
        public:

            TextureResources(Common::ILogger::Ptr logger,
                             PackageResourcesPtr packages,
                             std::shared_ptr<Render::IRenderer> renderer,
                             std::shared_ptr<Platform::IFiles> files,
                             std::shared_ptr<Platform::IText> text,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // ITextureResources
            //
            [[nodiscard]] std::future<Render::TextureId> LoadPackageTexture(
                const PackageResourceIdentifier& resource,
                const TextureLoadConfig& loadConfig,
                ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadPackageCubeTexture(
                const std::array<PackageResourceIdentifier, 6>& resources,
                const TextureLoadConfig& loadConfig,
                const std::string& tag,
                ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadCustomTexture(const Common::ImageData::Ptr& imageData,
                                                                           const TextureLoadConfig& loadConfig,
                                                                           const std::string& tag,
                                                                           ResultWhen resultWhen) override;
            [[nodiscard]] std::future<std::expected<TextRender, bool>> RenderText(const std::string& text,
                                                                                  const Platform::TextProperties& properties,
                                                                                  ResultWhen resultWhen) override;
            [[nodiscard]] std::optional<Render::Texture> GetLoadedTextureData(const Render::TextureId& textureId) const override;
            void DestroyTexture(const Render::TextureId& textureId) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] Render::TextureId OnLoadPackageTexture(
                const PackageResourceIdentifier& resource,
                const TextureLoadConfig& loadConfig,
                ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadPackageCubeTexture(
                const std::array<PackageResourceIdentifier, 6>& resources,
                const TextureLoadConfig& loadConfig,
                const std::string& tag,
                ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadCustomTexture(const Common::ImageData::Ptr& imageData,
                                                                const TextureLoadConfig& loadConfig,
                                                                const std::string& tag,
                                                                ResultWhen resultWhen);
            [[nodiscard]] std::expected<TextRender, bool> OnRenderText(const std::string& text,
                                                                       const Platform::TextProperties& properties,
                                                                       ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId LoadPackageTexture(const std::vector<PackageResourceIdentifier>& resources,
                                                               const TextureLoadConfig& loadConfig,
                                                               const std::string& tag,
                                                               ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId LoadTexture(const TextureData& textureData,
                                                        const TextureLoadConfig& loadConfig,
                                                        const std::string& tag,
                                                        ResultWhen resultWhen);

            [[nodiscard]] std::expected<Render::Texture, bool> ToRenderTexture(Render::TextureId textureId,
                                                                               const TextureData& textureData,
                                                                               const std::string& tag);
            [[nodiscard]] static Common::ImageData::Ptr TextureDataToImageData(const TextureData& textureData);

        private:

            Common::ILogger::Ptr m_logger;
            PackageResourcesPtr m_packages;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Platform::IText> m_text;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_texturesMutex;
            std::unordered_map<Render::TextureId, RegisteredTexture> m_textures;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H
