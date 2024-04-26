/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_ITEXTURERESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_ITEXTURERESOURCES_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/TextureData.h>

#include <Accela/Engine/Scene/TextRender.h>

#include <Accela/Render/Texture/Texture.h>

#include <Accela/Platform/Text/TextProperties.h>

#include <Accela/Render/Id.h>

#include <expected>
#include <string>
#include <memory>
#include <future>
#include <array>
#include <optional>

namespace Accela::Engine
{
    /**
     * Encapsulates texture resource operations
     */
    class ITextureResources
    {
        public:

            using Ptr = std::shared_ptr<ITextureResources>;

        public:

            virtual ~ITextureResources() = default;

            /**
             * Discovers all textures in the assets textures directory and asynchronously loads them into the engine.
             * Subsequent calls to GetAssetTextureId(..) should be made to discover the TextureIds associated with
             * the loaded textures.
             *
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled when the operation has finished
             */
            [[nodiscard]] virtual std::future<bool> LoadAllAssetTextures(ResultWhen resultWhen) = 0;

            /**
             * Asynchronously loads a specific texture file from the assets textures directory.
             * @param assetTextureName The file name of the texture to be loaded
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the TextureId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadAssetTexture(const std::string& assetTextureName, ResultWhen resultWhen) = 0;

            /**
             * Same as LoadAssetTexture, except takes in 6 file names, to be combined into a single cubic texture.
             *
             * @param assetTextureNames The texture file names for each face. Order is: left, right, top, bottom, near, far
             * @param tag A debug tag to associate with the texture
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the TextureId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadAssetCubeTexture(const std::array<std::string, 6>& assetTextureNames,
                                                                                      const std::string& tag,
                                                                                      ResultWhen resultWhen) = 0;

            /**
             * Asynchronously renders text and loads it into a texture.
             *
             * @param text The text to be rendered
             * @param properties The properties of how to render the text
             * @param resultWhen At which point of the load the returned future should be signaled
             * @return A future that's signaled with the TextureId when the operation has finished
             */
            [[nodiscard]] virtual std::future<std::expected<TextRender, bool>> RenderText(const std::string& text,
                                                                                          const Platform::TextProperties& properties,
                                                                                          ResultWhen resultWhen) = 0;

            /**
             * Returns the TextureId of a previously loaded assets texture.
             *
             * @param assetTextureName The file name of the assets texture in question
             *
             * @return The TextureId, or std::nullopt if no such texture exists
             */
            [[nodiscard]] virtual std::optional<Render::TextureId> GetAssetTextureId(const std::string& assetTextureName) const = 0;

            /**
             * Retrieves texture details about a previously loaded texture.
             *
             * @param textureId The TextureId of the texture in question
             *
             * @return The texture's details, or std::nullopt if no such texture
             */
            [[nodiscard]] virtual std::optional<Render::Texture> GetLoadedTextureData(const Render::TextureId& textureId) const = 0;

            /**
             * Destroy a previously loaded texture
             *
             * @param textureId The TextureId of the texture to be deleted
             */
            virtual void DestroyTexture(const Render::TextureId& textureId) = 0;

            /**
             * Destroy all previously loaded textures
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_ITEXTURERESOURCES_H
