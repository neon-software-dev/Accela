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

            [[nodiscard]] std::string GetAccelaDirectory() const override;
            [[nodiscard]] std::string GetAccelaSubdirectory(const std::string& subDirName) const override;
            [[nodiscard]] std::string GetAccelaFilePath(const std::string& subdir, const std::string& fileName) const override;
            [[nodiscard]] std::expected<std::vector<std::string>, bool> ListFilesInAccelaSubdir(const std::string& subdir) const override;

            [[nodiscard]] std::string GetPackagesDirectory() const override;
            [[nodiscard]] std::string GetPackageDirectory(const std::string& packageName) const override;

            [[nodiscard]] std::expected<PackageSource::Ptr, bool> LoadPackage(const std::string& packageName) const override;

            [[nodiscard]] std::string GetSubdirPath(const std::string& root, const std::string& subdir) const override;
            [[nodiscard]] std::string EnsureEndsWithSeparator(const std::string& source) const override;

            [[nodiscard]] std::expected<std::vector<std::string>, bool> ListFilesInDirectory(const std::string& directory) const override;
            [[nodiscard]] std::expected<Common::ImageData::Ptr, bool> LoadTexture(const std::vector<std::byte>& data, const std::optional<std::string>& dataFormatHint) const override;
            [[nodiscard]] std::expected<std::vector<unsigned char>, bool> LoadAccelaFile(const std::string& subdir, const std::string& fileName) const override;

        private:

            [[nodiscard]] static std::string GetExecutableDirectory();

            [[nodiscard]] std::expected<Common::ImageData::Ptr, unsigned int> LoadTexture(const std::string& filePath) const;

        private:

            Common::ILogger::Ptr m_logger;
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_FILE_SDLFILES_H
