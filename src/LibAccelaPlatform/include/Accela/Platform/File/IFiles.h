#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_FILE_IFILES_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_FILE_IFILES_H

#include <Accela/Platform/Package/Package.h>

#include <Accela/Common/ImageData.h>

#include <expected>
#include <string>
#include <memory>
#include <vector>
#include <cstddef>
#include <optional>

namespace Accela::Platform
{
    static constexpr const char* ACCELA_DIR = "accela";
    static constexpr const char* ASSETS_DIR = "assets";
    static constexpr const char* PACKAGES_DIR = "packages";
    static constexpr const char* CONSTRUCTS_DIR = "constructs";
    static constexpr const char* SHADERS_SUBDIR = "shaders";
    static constexpr const char* TEXTURES_SUBDIR = "textures";
    static constexpr const char* AUDIO_SUBDIR = "audio";
    static constexpr const char* FONTS_SUBDIR = "fonts";
    static constexpr const char* MODELS_SUBDIR = "models";

    static constexpr const char* PACKAGE_EXTENSION = ".acp";
    static constexpr const char* CONSTRUCT_EXTENSION = ".acc";

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

            [[nodiscard]] virtual std::string GetAccelaDirectory() const = 0;
            [[nodiscard]] virtual std::string GetAccelaSubdirectory(const std::string& subDirName) const = 0;
            [[nodiscard]] virtual std::string GetAccelaFilePath(const std::string& subdir, const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::string>, bool> ListFilesInAccelaSubdir(const std::string& subdir) const = 0;

            [[nodiscard]] virtual std::string GetPackagesDirectory() const = 0;
            [[nodiscard]] virtual std::string GetPackageDirectory(const std::string& packageName) const = 0;

            [[nodiscard]] virtual std::expected<Package::Ptr, bool> LoadPackage(const std::string& packageName) const = 0;

            [[nodiscard]] virtual std::string GetSubdirPath(const std::string& root, const std::string& subdir) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::string>, bool> ListFilesInDirectory(const std::string& directory) const = 0;
            [[nodiscard]] virtual std::string EnsureEndsWithSeparator(const std::string& source) const = 0;

            [[nodiscard]] virtual std::expected<Common::ImageData::Ptr, bool> LoadCompressedTexture(const std::vector<std::byte>& data, const std::size_t& dataByteSize, const std::optional<std::string>& dataFormatHint) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<unsigned char>, bool> LoadAccelaFile(const std::string& subdir, const std::string& fileName) const = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_FILE_IFILES_H
