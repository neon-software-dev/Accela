#ifndef LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PACKAGE_DISKPACKAGE_H
#define LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PACKAGE_DISKPACKAGE_H

#include <Accela/Platform/Package/Package.h>

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace Accela::Platform
{
    /**
     * A package located on disk, accessible via standard C++ fstream/filesystem functions
     */
    class DiskPackage : public Package
    {
        public:

            enum class CreateOnDiskError
            {
                DirectoryDoesntExist,
                PackageFileAlreadyExists,
                FailedToCreateDirectory,
                FailedToCreateSubdirectory,
                FailedToCreatePackageFile,
                FailedToCreateConstructFile
            };

            enum class OpenPackageError
            {
                PackageFileDoesntExist,
                PackageStructureBroken,
                FailureLoadingMetadata
            };

            // TODO: Other error enums for Get..() methods rather than returning ints

        public:

            /**
             * Creates a stub package on disk. Will create a folder for the package, which contains folders
             * for assets and constructs, a default package file, and a default construct.
             *
             * @param dir The directory in which to create the package
             * @param packageName The name of the package
             *
             * @return The path to the created package file, or CreateOnDiskError on error
             */
            [[nodiscard]] static std::expected<std::filesystem::path, CreateOnDiskError> CreateOnDisk(
                const std::filesystem::path& dir,
                const std::string& packageName);

            /**
             * Opens and reads a package on disk
             *
             * @param packageFile The path to the package's package file
             *
             * @return A Package object, or OpenPackageError on error
             */
            [[nodiscard]] static std::expected<Package::Ptr, OpenPackageError> OpenOnDisk(
                const std::filesystem::path& packageFile);

        private:

            struct Tag{};

        public:

            DiskPackage(Tag, std::filesystem::path packageDir, std::filesystem::path packageFilePath);

            auto operator<=>(const DiskPackage&) const = default;

            [[nodiscard]] std::vector<std::string> GetAudioFileNames() const override;
            [[nodiscard]] std::vector<std::string> GetFontFileNames() const override;
            [[nodiscard]] std::vector<std::string> GetModelFileNames() const override;
            [[nodiscard]] std::vector<std::string> GetTextureFileNames() const override;
            [[nodiscard]] std::vector<std::string> GetConstructFileNames() const override;

            [[nodiscard]] std::expected<std::vector<unsigned char>, unsigned int> GetFontData(const std::string& fileName) const override;
            [[nodiscard]] std::expected<std::vector<unsigned char>, unsigned int> GetAudioData(const std::string& fileName) const override;
            [[nodiscard]] std::expected<std::vector<unsigned char>, unsigned int> GetModelData(const std::string& fileName) const override;
            [[nodiscard]] std::expected<Common::ImageData::Ptr, unsigned int> GetTextureData(const std::string& fileName) const override;
            [[nodiscard]] std::expected<Common::ImageData::Ptr, unsigned int> GetModelTextureData(const std::string& modelFileName,
                                                                                                  const std::string& textureFileName) const override;

        private:

            [[nodiscard]] bool LoadMetadata();

            [[nodiscard]] static std::expected<std::vector<std::filesystem::path>, bool> GetFilePaths(const std::filesystem::path& directory);
            [[nodiscard]] static std::expected<std::vector<std::filesystem::path>, bool> GetModelFilePaths(const std::filesystem::path& directory);
            [[nodiscard]] static std::vector<std::string> GetFileNames(const std::vector<std::filesystem::path>& filePaths);
            [[nodiscard]] static std::expected<Common::ImageData::Ptr, unsigned int> GetTextureData(const std::filesystem::path& filePath);
            [[nodiscard]] static std::expected<std::vector<unsigned char>, bool> GetFileBytes(const std::filesystem::path& filePath);

        private:

            std::filesystem::path m_packageDir;
            std::filesystem::path m_packageFilePath;

            //
            // Package structure
            //
            std::vector<std::filesystem::path> m_audioAssets;
            std::vector<std::filesystem::path> m_fontAssets;
            std::vector<std::filesystem::path> m_modelAssets;
            std::vector<std::filesystem::path> m_textureAssets;
            std::vector<std::filesystem::path> m_constructs;
    };
}

#endif //LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_PACKAGE_DISKPACKAGE_H
