/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_FILE_IFILES_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_FILE_IFILES_H

#include <Accela/Common/ImageData.h>

#include <assimp/Importer.hpp>

#include <expected>
#include <string>
#include <memory>
#include <vector>

namespace Accela::Platform
{
    static constexpr const char* ASSETS_DIR = "assets";
    static constexpr const char* SHADERS_SUBDIR = "shaders";
    static constexpr const char* TEXTURES_SUBDIR = "textures";
    static constexpr const char* AUDIO_SUBDIR = "audio";
    static constexpr const char* FONTS_DIR = "fonts";
    static constexpr const char* MODELS_DIR = "models";

    /**
     * Interface to accessing engine files from disk on PC, APK assets on Android.
     *
     * Warning: All implemented methods must be fully thread safe. Asset loading is done
     * asynchronously from multiple threads in parallel.
     */
    class IFiles
    {
        public:

            using Ptr = std::shared_ptr<IFiles>;

        public:

            virtual ~IFiles() = default;

            [[nodiscard]] virtual std::string GetAssetsDirectory() const = 0;
            [[nodiscard]] virtual std::string GetAssetsSubdirectory(const std::string& subDirName) const = 0;
            [[nodiscard]] virtual std::string GetAssetFilePath(const std::string& subdir, const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::string GetSubdirPath(const std::string& root, const std::string& subdir) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::string>, bool> ListFilesInAssetsSubdir(const std::string& subdir) const = 0;
            [[nodiscard]] virtual std::expected<Common::ImageData::Ptr, unsigned int> LoadAssetTexture(const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<Common::ImageData::Ptr, unsigned int> LoadAssetModelTexture(const std::string& modelName, const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<unsigned char>, bool> LoadAssetFile(const std::string& subdir, const std::string& fileName) const = 0;

            /**
             * The Assimp library access files on disk through an internal IOSystem interface that can
             * be provided by the client. The default IOSystem uses standard c-funcs to access disk files.
             * However, some platforms, such as Android, have different requirements, such as the ability
             * to load files out of an APK's assets. For such platforms, they can override this method
             * and configure the Assimp Importer to use a custom IOSystem.
             *
             * @param importer The Assimp importer to be configured
             */
            virtual void ConfigureAssimpImporter(Assimp::Importer& importer) const { (void) importer; }
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_FILE_IFILES_H
