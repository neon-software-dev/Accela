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
#include <Accela/Render/Id.h>

#include <Accela/Common/ImageData.h>

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
            * Loads a texture resource from a package.
            *
            * @param resource Identifies the texture
            * @param resultWhen At which point of the load the returned future should be signaled
            *
            * @return A future containing the TextureId of the loaded texture, or INVALID_ID on error
            */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadTexture(
                const PackageResourceIdentifier& resource,
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
            [[nodiscard]] virtual std::future<Render::TextureId> LoadCubeTexture(
                const std::array<PackageResourceIdentifier, 6>& resources,
                const std::string& tag,
                ResultWhen resultWhen) = 0;

            /**
            * Loads a custom texture resource.
            *
            * @param resource Identifies the texture
            * @param imageData The texture data to be loaded
            * @param resultWhen At which point of the load the returned future should be signaled
            *
            * @return A future containing the TextureId of the loaded texture, or INVALID_ID on error
            */
            [[nodiscard]] virtual std::future<Render::TextureId> LoadTexture(
                const CustomResourceIdentifier& resource,
                const Common::ImageData::Ptr& imageData,
                ResultWhen resultWhen) = 0;

            /**
             * Loads all texture resources from the specified package
             *
             * @param packageName Identifies the package
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing whether all textures loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadAllTextures(const PackageName& packageName, ResultWhen resultWhen) = 0;

            /**
             * Loads all texture resources across all registered packages.
             *
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future containing whether all textures loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadAllTextures(ResultWhen resultWhen) = 0;

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
             * Returns the TextureId of a previously loaded texture.
             *
             * @param resource Identifies the texture
             *
             * @return The TextureId, or std::nullopt if no such texture exists
             */
            [[nodiscard]] virtual std::optional<Render::TextureId> GetTextureId(const ResourceIdentifier& resource) const = 0;

            /**
             * Retrieves texture data about a previously loaded texture.
             *
             * @param resource Identifies the texture
             *
             * @return The texture's details, or std::nullopt if no such texture
             */
            [[nodiscard]] virtual std::optional<Render::Texture> GetLoadedTextureData(const ResourceIdentifier& resource) const = 0;

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
             * @param resource Identifies the texture
             */
            virtual void DestroyTexture(const ResourceIdentifier& resource) = 0;

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
