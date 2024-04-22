/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_ENGINEASSETS_H
#define LIBACCELAENGINE_SRC_ENGINEASSETS_H

#include "Model/ModelLoader.h"

#include <Accela/Engine/IEngineAssets.h>

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Engine
{
    class EngineAssets : public IEngineAssets
    {
        public:

            EngineAssets(Common::ILogger::Ptr logger, Platform::IFiles::Ptr files);

            [[nodiscard]] std::expected<TextureData, bool> ReadTextureBlocking(const std::string& textureName) const override;
            [[nodiscard]] std::expected<TextureData, bool> ReadCubeTextureBlocking(const std::array<std::string, 6>& textureNames) const override;
            [[nodiscard]] std::expected<Common::AudioData::Ptr, bool> ReadAudioBlocking(const std::string& audioName) const override;
            [[nodiscard]] std::expected<Model::Ptr, bool> ReadModelBlocking(const std::string& modelName, const std::string& modelExtension) const override;

        private:

            Common::ILogger::Ptr m_logger;
            Platform::IFiles::Ptr m_files;
            ModelLoader m_modelLoader;
    };
}

#endif //LIBACCELAENGINE_SRC_ENGINEASSETS_H
