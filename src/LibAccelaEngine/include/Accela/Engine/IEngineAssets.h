/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINEASSETS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINEASSETS_H

#include "TextureData.h"

#include <Accela/Engine/Model/Model.h>

#include <Accela/Common/ImageData.h>
#include <Accela/Common/AudioData.h>

#include <expected>
#include <memory>
#include <string>
#include <array>

namespace Accela::Engine
{
    /**
     * Provides an interface for reading into memory the bundled assets which the engine has access to
     */
    class IEngineAssets
    {
        public:

            using Ptr = std::shared_ptr<IEngineAssets>;

        public:

            virtual ~IEngineAssets() = default;

            /**
             * Blocking call to read the specified texture from the assets textures directory into memory.
             *
             * @param textureName The file name of the texture to be read
             *
             * @return The data of the texture, or false on error
             */
            [[nodiscard]] virtual std::expected<TextureData, bool> ReadTextureBlocking(const std::string& textureName) const = 0;

            /**
             * Blocking call to read the specified cube-mapped texture from the assets textures directory into memory.
             *
             * @param textureName The file names of the cube texture to be read. Order is: left, right, top, bottom, near, far
             *
             * @return The data of the texture, or false on error
             */
            [[nodiscard]] virtual std::expected<TextureData, bool> ReadCubeTextureBlocking(const std::array<std::string, 6>& textureNames) const = 0;

            /**
             * Blocking call to read the specified audio file from from the assets audio directory into memory.
             *
             * @param audioName The file name of the audio the be read
             *
             * @return The data of the audio, or false on error
             */
            [[nodiscard]] virtual std::expected<Common::AudioData::Ptr, bool> ReadAudioBlocking(const std::string& audioName) const = 0;

            /**
             * Blocking call to read the specified model file from from the assets models directory into memory.
             *
             * Note that assets model directory requires a specific directory for each model, given the same name (minus
             * extension) of the model file that's contained directly within that directory.
             *
             * @param modelName The directory name of the model to be read
             * @param modelExtension The file extension of the model file
             *
             * @return The data of the model, or false on error
             */
            [[nodiscard]] virtual std::expected<Model::Ptr, bool> ReadModelBlocking(const std::string& modelName, const std::string& modelExtension) const = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINEASSETS_H
