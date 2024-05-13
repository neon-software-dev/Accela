/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H

#include <Accela/Engine/Scene/ITextureResources.h>

#include "../Texture/RegisteredTexture.h"

#include <Accela/Common/Log/ILogger.h>

#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <expected>
#include <memory>
#include <future>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <optional>

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
    class IEngineAssets;

    class TextureResources : public ITextureResources
    {
        public:

            TextureResources(Common::ILogger::Ptr logger,
                             std::shared_ptr<Render::IRenderer> renderer,
                             std::shared_ptr<IEngineAssets> assets,
                             std::shared_ptr<Platform::IFiles> files,
                             std::shared_ptr<Platform::IText> text,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // ITextureResources
            //
            [[nodiscard]] std::future<bool> LoadAllAssetTextures(ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadAssetTexture(const std::string& assetTextureName, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadAssetCubeTexture(const std::array<std::string, 6>& assetTextureNames, const std::string& tag, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadTexture(const Common::ImageData::Ptr& imageData, const std::string& tag, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<std::expected<TextRender, bool>> RenderText(const std::string& text, const Platform::TextProperties& properties, ResultWhen resultWhen) override;
            [[nodiscard]] std::optional<Render::TextureId> GetAssetTextureId(const std::string& assetTextureName) const override;
            [[nodiscard]] std::optional<Render::Texture> GetLoadedTextureData(const Render::TextureId& textureId) const override;
            void DestroyTexture(const Render::TextureId& textureId) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] bool OnLoadAllAssetTextures(ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadAssetTexture(const std::string& assetTextureName, ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadAssetCubeTexture(const std::array<std::string, 6>& assetTextureNames, const std::string& tag, ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadTexture(const Common::ImageData::Ptr& imageData, const std::string& tag, ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadAssetTextureInternal(const std::vector<std::string>& assetTextureNames, const std::string& tag, ResultWhen resultWhen);
            [[nodiscard]] std::expected<TextRender, bool> OnRenderText(const std::string& text, const Platform::TextProperties& properties, ResultWhen resultWhen);

            [[nodiscard]] static Render::Texture ToRenderTexture(Render::TextureId textureId,
                                                                 const TextureData& textureData,
                                                                 const std::string& tag);
            [[nodiscard]] static Common::ImageData::Ptr TextureDataToImageData(const TextureData& textureData);

            [[nodiscard]] static std::size_t GetAssetHash(const std::vector<std::string>& fileNames);

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Platform::IText> m_text;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_assetsMutex;
            std::unordered_map<std::size_t, Render::TextureId> m_assetToTexture;
            std::unordered_map<Render::TextureId, std::size_t> m_textureToAsset;

            mutable std::mutex m_texturesMutex;
            std::unordered_map<Render::TextureId, RegisteredTexture> m_textures;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H
