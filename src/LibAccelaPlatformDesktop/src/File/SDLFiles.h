/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_FILE_SDLFILES_H
#define LIBACCELAPLATFORMSDL_SRC_FILE_SDLFILES_H

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Log/ILogger.h>

#include <string>

class SDL_Surface;

namespace Accela::Platform
{
    class SDLFiles : public IFiles
    {
        public:

            explicit SDLFiles(Common::ILogger::Ptr logger);

            [[nodiscard]] std::string GetAssetsDirectory() const override;
            [[nodiscard]] std::string GetAssetsSubdirectory(const std::string& subDirName) const override;
            [[nodiscard]] std::string GetAssetFilePath(const std::string& subdir, const std::string& fileName) const override;
            [[nodiscard]] std::string GetSubdirPath(const std::string& root, const std::string& subdir) const override;
            [[nodiscard]] std::expected<std::vector<std::string>, bool> ListFilesInAssetsSubdir(const std::string& subdir) const override;
            [[nodiscard]] std::expected<Common::ImageData::Ptr, unsigned int> LoadAssetTexture(const std::string& fileName) const override;
            [[nodiscard]] std::expected<Common::ImageData::Ptr, unsigned int> LoadAssetModelTexture(const std::string& modelName, const std::string& fileName) const override;
            [[nodiscard]] std::expected<Common::ImageData::Ptr, bool> LoadCompressedTexture(const std::vector<std::byte>& data, const std::size_t& dataByteSize, const std::optional<std::string>& dataFormatHint) const override;
            [[nodiscard]] std::expected<std::vector<unsigned char>, bool> LoadAssetFile(const std::string& subdir, const std::string& fileName) const override;

        private:

            [[nodiscard]] static std::string GetExecutableDirectory();
            [[nodiscard]] static std::string EnsureEndsWithSeparator(const std::string& source);

            [[nodiscard]] std::expected<Common::ImageData::Ptr, unsigned int> LoadTexture(const std::string& filePath) const;

            static Common::ImageData::Ptr SDLSurfaceToImageData(const Common::ILogger::Ptr& logger, SDL_Surface *pSurface);

        private:

            Common::ILogger::Ptr m_logger;
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_FILE_SDLFILES_H
