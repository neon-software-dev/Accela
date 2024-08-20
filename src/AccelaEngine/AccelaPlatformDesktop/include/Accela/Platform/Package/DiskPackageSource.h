/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PACKAGE_DISKPACKAGESOURCE_H
#define LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PACKAGE_DISKPACKAGESOURCE_H

#include "Accela/Platform/Package/PackageSource.h"

#include <Accela/Common/SharedLib.h>

#include <expected>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>

namespace Accela::Platform
{
    /**
     * A package located on disk, accessible via standard C++ fstream/filesystem functions
     */
    class ACCELA_PUBLIC DiskPackageSource : public PackageSource
    {
        public:

            using Ptr = std::shared_ptr<DiskPackageSource>;

            enum class OpenPackageError
            {
                PackageFileDoesntExist,
                PackageStructureBroken,
                FailureLoadingMetadata
            };

            // TODO: Other error enums for Get..() methods rather than returning ints

        public:

            /**
             * Opens and reads a package on disk
             *
             * @param manifestFile The path to the package's manifest file
             *
             * @return A PackageSource object, or OpenPackageError on error
             */
            [[nodiscard]] static std::expected<PackageSource::Ptr, OpenPackageError> OpenOnDisk(
                const std::filesystem::path& manifestFile);

        private:

            struct Tag{};

        public:

            DiskPackageSource(Tag, std::filesystem::path packageDir, std::filesystem::path packageFilePath);

            auto operator<=>(const DiskPackageSource&) const = default;

            //
            // Package
            //
            [[nodiscard]] std::vector<std::string> GetAudioResourceNames() const override;
            [[nodiscard]] std::vector<std::string> GetFontResourceNames() const override;
            [[nodiscard]] std::vector<std::string> GetModelResourceNames() const override;
            [[nodiscard]] std::vector<std::string> GetTextureResourceNames() const override;
            [[nodiscard]] std::vector<std::string> GetConstructResourceNames() const override;

            [[nodiscard]] std::expected<std::string, bool> GetTextureFormatHint(const std::string& resourceName) const override;
            [[nodiscard]] std::expected<std::string, bool> GetModelTextureFormatHint(const std::string& modelResourceName,
                                                                                     const std::string& resourceName) const override;

            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetManifestFileData() const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetFontData(const std::string& resourceName) const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetAudioData(const std::string& resourceName) const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetModelData(const std::string& resourceName) const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetTextureData(const std::string& resourceName) const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetModelTextureData(const std::string& modelResourceName,
                                                                                                  const std::string& textureResourceName) const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, unsigned int> GetConstructData(const std::string& constructName) const override;

            //
            // Internal
            //
            [[nodiscard]] std::filesystem::path GetPackageDir() const noexcept { return m_packageDir; }
            [[nodiscard]] std::filesystem::path GetManifestFilePath() const noexcept { return m_manifestFilePath; }

        private:

            [[nodiscard]] bool LoadMetadata();

            [[nodiscard]] static std::expected<std::vector<std::filesystem::path>, bool> GetFilePaths(const std::filesystem::path& directory);
            [[nodiscard]] static std::expected<std::vector<std::filesystem::path>, bool> GetModelFilePaths(const std::filesystem::path& directory);
            [[nodiscard]] static std::vector<std::string> GetFileNames(const std::vector<std::filesystem::path>& filePaths);
            [[nodiscard]] static std::expected<std::vector<std::byte>, bool> GetFileBytes(const std::filesystem::path& filePath);

        private:

            std::filesystem::path m_packageDir;
            std::filesystem::path m_manifestFilePath;

            //
            // PackageSource structure
            //
            std::vector<std::filesystem::path> m_audioAssets;
            std::vector<std::filesystem::path> m_fontAssets;
            std::vector<std::filesystem::path> m_modelAssets;
            std::vector<std::filesystem::path> m_textureAssets;
            std::vector<std::filesystem::path> m_constructs;
    };
}

#endif //LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PACKAGE_DISKPACKAGESOURCE_H
