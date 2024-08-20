/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_ITEXTURERESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_ITEXTURERESOURCES_H

#include <Accela/Engine/ResourceIdentifier.h>
#include <Accela/Engine/TextureData.h>

#include <Accela/Engine/Scene/TextRender.h>

#include <Accela/Platform/Text/TextProperties.h>

#include <Accela/Render/Texture/Texture.h>
#include <Accela/Render/Texture/TextureSampler.h>
#include <Accela/Render/Id.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/ImageData.h>

#include <expected>
#include <string>
#include <memory>
#include <future>
#include <array>
#include <optional>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC TextureLoadConfig
    {
        std::optional<unsigned int> numMipLevels;
        std::optional<Render::UVAddressMode> uvAddressMode;
    };

    /**
     * Encapsulates texture resource operations
     */
    class ACCELA_PUBLIC ITextureResources
    {
        public:

            using Ptr = std::shared_ptr<ITextureResources>;

        public:

            virtual ~ITextureResources() = default;

            /**
            * Loads a texture resource from a package.
            *
            * @param resource Identifies the texture
            * @param resultWhen At which point of the load the returned future should be signaled
            *
            * @return A future containing the TextureId of the loaded texture, or INVALID_ID on error
            */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadPackageTexture(
                const PackageResourceIdentifier& resource,
                const TextureLoadConfig& loadConfig,
                ResultWhen resultWhen) = 0;

            /**
            * Loads a cube texture resource from a package.
            *
            * @param resources Identifies the cube texture (Right, Left, Up, Down, Back, Forward)
            * @param tag A debug tag to associate with the texture
            * @param resultWhen At which point of the load the returned future should be signaled
            *
            * @return A future containing the TextureId of the loaded texture, or INVALID_ID on error
            */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadPackageCubeTexture(
                const std::array<PackageResourceIdentifier, 6>& resources,
                const TextureLoadConfig& loadConfig,
                const std::string& tag,
                ResultWhen resultWhen) = 0;

            /**
            * Loads a custom texture resource.
            *
            * @param imageData The texture data to be loaded
            * @param resultWhen At which point of the load the returned future should be signaled
            *
            * @return A future containing the TextureId of the loaded texture, or INVALID_ID on error
            */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadCustomTexture(
                const Common::ImageData::Ptr& imageData,
                const TextureLoadConfig& loadConfig,
                const std::string& tag,
                ResultWhen resultWhen) = 0;

            /**
             * Asynchronously renders text and loads it into a texture.
             *
             * @param text The text to be rendered
             * @param properties The properties of how to render the text
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing details of the rendered text, or std::unexpected on error
             */
            [[nodiscard]] virtual std::future<std::expected<TextRender, bool>> RenderText(const std::string& text,
                                                                                          const Platform::TextProperties& properties,
                                                                                          ResultWhen resultWhen) = 0;

            /**
             * Retrieves texture data about a previously loaded texture.
             *
             * @param textureId Identifies the texture
             *
             * @return The texture's details, or std::nullopt if no such texture
             */
            [[nodiscard]] virtual std::optional<Render::Texture> GetLoadedTextureData(const Render::TextureId& textureId) const = 0;

            /**
             * Destroy a previously loaded texture resource
             *
             * @param textureId Identifies the texture
             */
            virtual void DestroyTexture(const Render::TextureId& textureId) = 0;

            /**
             * Destroy all previously loaded texture resources
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_ITEXTURERESOURCES_H
