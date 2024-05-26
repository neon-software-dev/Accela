/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/Package/DiskPackage.h>

#include "../SDLUtil.h"

#include <Accela/Platform/File/IFiles.h>

#include <SDL_image.h>

#include <nlohmann/json.hpp>

#include <fstream>

namespace Accela::Platform
{

std::expected<std::filesystem::path, DiskPackage::CreateOnDiskError> DiskPackage::CreateOnDisk(
    const std::filesystem::path& dir,
    const std::string& packageName)
{
    std::error_code ec{};

    // Package Directory (e.g. '/path/to/packages/PackageName')
    const auto packageDir = dir / std::filesystem::path(packageName);

    // FileName of the package file (e.g. 'PackageName.acp')
    const auto packageFileName = packageName + Platform::PACKAGE_EXTENSION;

    // Full path to the package file on disk
    auto packageFilePath = packageDir / packageFileName;

    // If the dir to create the package in doesn't exist, bail out
    if (!std::filesystem::exists(dir))
    {
        return std::unexpected(CreateOnDiskError::DirectoryDoesntExist);
    }

    // If the package directory already exists, bail out
    if (std::filesystem::exists(packageDir, ec) || ec)
    {
        return std::unexpected(CreateOnDiskError::PackageFileAlreadyExists);
    }

    // Create the package directory
    if (!std::filesystem::create_directory(packageDir, ec) || ec)
    {
        return std::unexpected(CreateOnDiskError::FailedToCreateDirectory);
    }

    // Create the package's directories
    const auto packageAssetsPath = packageDir / Platform::ASSETS_DIR;

    std::vector<std::filesystem::path> subDirectories = {
        // Assets subdirectories
        packageAssetsPath / Platform::AUDIO_SUBDIR,
        packageAssetsPath / Platform::FONTS_SUBDIR,
        packageAssetsPath / Platform::MODELS_SUBDIR,
        packageAssetsPath / Platform::TEXTURES_SUBDIR,
        // Construct subdirectory
        packageDir / Platform::CONSTRUCTS_DIR,
    };

    for (const auto& subDir : subDirectories)
    {
        // Note that create_directories creates upper/higher directories as needed.
        if (!std::filesystem::create_directories(subDir, ec) || ec)
        {
            return std::unexpected(CreateOnDiskError::FailedToCreateSubdirectory);
        }
    }

    // Create the root package file
    {
        std::ofstream ofs;
        ofs.open(packageFilePath, std::ofstream::out);

        if (!ofs.good())
        {
            return std::unexpected(CreateOnDiskError::FailedToCreatePackageFile);
        }

        nlohmann::json defaultJson{
            {Package::VERSION_KEY, Package::VERSION}
        };
        ofs << std::setw(4) << defaultJson << std::endl;

        ofs.close();
    }

    // Create a default construct
    {
        const auto defaultConstructPath =
            packageDir / Platform::CONSTRUCTS_DIR / ("default" + std::string(Platform::CONSTRUCT_EXTENSION));

        std::ofstream ofs;
        ofs.open(defaultConstructPath, std::ofstream::out);

        if (!ofs.good())
        {
            return std::unexpected(CreateOnDiskError::FailedToCreateConstructFile);
        }

        nlohmann::json defaultJson{};
        ofs << std::setw(4) << defaultJson << std::endl;

        ofs.close();
    }

    return packageFilePath;
}

std::expected<Package::Ptr, DiskPackage::OpenPackageError> DiskPackage::OpenOnDisk(const std::filesystem::path& packageFile)
{
    //
    // The package file should exist
    //
    if (!std::filesystem::exists(packageFile))
    {
        return std::unexpected(OpenPackageError::PackageFileDoesntExist);
    }

    //
    // Subdirectories for assets and constructs should exist
    //
    const auto packageDir = packageFile.parent_path();

    if (!std::filesystem::exists(packageDir / Platform::ASSETS_DIR))
    {
        return std::unexpected(OpenPackageError::PackageStructureBroken);
    }
    if (!std::filesystem::exists(packageDir / Platform::CONSTRUCTS_DIR))
    {
        return std::unexpected(OpenPackageError::PackageStructureBroken);
    }

    //
    // Create the package object and have it load its metadata from the package
    //
    auto package = std::make_shared<DiskPackage>(Tag{}, packageDir, packageFile);

    if (!package->LoadMetadata())
    {
        return std::unexpected(OpenPackageError::FailureLoadingMetadata);
    }

    return package;
}

DiskPackage::DiskPackage(DiskPackage::Tag, std::filesystem::path packageDir, std::filesystem::path packageFilePath)
    : Package(packageFilePath.filename().replace_extension())
    , m_packageDir(std::move(packageDir))
    , m_packageFilePath(std::move(packageFilePath))
{

}

std::vector<std::string> DiskPackage::GetAudioFileNames() const
{
    return GetFileNames(m_audioAssets);
}

std::vector<std::string> DiskPackage::GetFontFileNames() const
{
    return GetFileNames(m_fontAssets);
}

std::vector<std::string> DiskPackage::GetModelFileNames() const
{
    return GetFileNames(m_modelAssets);
}

std::vector<std::string> DiskPackage::GetTextureFileNames() const
{
    return GetFileNames(m_textureAssets);
}

std::vector<std::string> DiskPackage::GetConstructFileNames() const
{
    return GetFileNames(m_constructs);
}

bool DiskPackage::LoadMetadata()
{
    //
    // Get lists of asset files
    //
    const auto assetsDir = m_packageDir / Platform::ASSETS_DIR;

    const auto audioAssets = GetFilePaths(assetsDir / Platform::AUDIO_SUBDIR);
    if (!audioAssets) { return false; }
    m_audioAssets = *audioAssets;

    const auto fontAssets = GetFilePaths(assetsDir / Platform::FONTS_SUBDIR);
    if (!fontAssets) { return false; }
    m_fontAssets = *fontAssets;

    const auto modelAssets = GetModelFilePaths(assetsDir / Platform::MODELS_SUBDIR);
    if (!modelAssets) { return false; }
    m_modelAssets = *modelAssets;

    const auto textureAssets = GetFilePaths(assetsDir / Platform::TEXTURES_SUBDIR);
    if (!textureAssets) { return false; }
    m_textureAssets = *textureAssets;

    //
    // Get lists of constructs
    //
    const auto constructsDir = m_packageDir / Platform::CONSTRUCTS_DIR;

    const auto constructs = GetFilePaths(constructsDir);
    if (!constructs) { return false; }
    m_constructs = *constructs;

    return true;
}

std::expected<std::vector<std::filesystem::path>, bool> DiskPackage::GetFilePaths(const std::filesystem::path& directory)
{
    std::error_code ec{};

    std::vector<std::filesystem::path> filePaths;

    for (const auto& dirEntry: std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) { return std::unexpected(false); }

        filePaths.push_back(dirEntry.path());
    }

    return filePaths;
}

std::expected<std::vector<std::filesystem::path>, bool> DiskPackage::GetModelFilePaths(const std::filesystem::path& directory)
{
    std::error_code ec{};

    std::vector<std::filesystem::path> filePaths;

    // Iterate over all the base model directories with assets/models
    for (const auto& dirEntry: std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) { return std::unexpected(false); }

        // Ignore non-directories within the assets/models directory
        if (!dirEntry.is_directory(ec) || ec)
        {
            continue;
        }

        const auto dirName = dirEntry.path().filename();

        // Iterate over the model directory's contents
        for (const auto& modelDirEntry: std::filesystem::directory_iterator(dirEntry, ec))
        {
            if (ec) { return std::unexpected(false); }

            // Skip directories
            if (modelDirEntry.is_directory(ec) || ec)
            {
                continue;
            }

            // If the file has the same name as the directory, it's the model file
            if (modelDirEntry.path().filename().replace_extension() == dirName)
            {
                filePaths.push_back(modelDirEntry.path());
                break;
            }
        }
    }

    return filePaths;
}

std::vector<std::string> DiskPackage::GetFileNames(const std::vector<std::filesystem::path>& filePaths)
{
    std::vector<std::string> names;

    std::ranges::transform(filePaths, std::back_inserter(names), [](const auto& filePath){
        return filePath.filename();
    });

    return names;
}

std::expected<std::vector<unsigned char>, bool> DiskPackage::GetFileBytes(const std::filesystem::path& filePath)
{
    std::error_code ec{};

    if (!std::filesystem::exists(filePath, ec) || ec)
    {
        return std::unexpected(0);
    }

    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected(false);
    }

    file.seekg(0, std::ios::end);
    std::vector<unsigned char> fileContents(file.tellg());
    file.seekg(0, std::ios::beg);
    fileContents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return fileContents;
}

std::expected<Common::ImageData::Ptr, unsigned int> DiskPackage::GetTextureData(const std::filesystem::path& filePath)
{
    std::error_code ec{};

    if (!std::filesystem::exists(filePath, ec) || ec)
    {
        return std::unexpected(0);
    }

    SDL_Surface *pSurface = IMG_Load(filePath.c_str());
    if (pSurface == nullptr)
    {
        return std::unexpected(1);
    }

    auto imageData = SDLUtil::SDLSurfaceToImageData(pSurface);
    if (imageData == nullptr)
    {
        return std::unexpected(2);
    }

    return imageData;
}

std::expected<std::vector<unsigned char>, unsigned int> DiskPackage::GetFontData(const std::string& fileName) const
{
    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::FONTS_SUBDIR / fileName);
}

std::expected<std::vector<unsigned char>, unsigned int> DiskPackage::GetAudioData(const std::string& fileName) const
{
    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::AUDIO_SUBDIR / fileName);
}

std::expected<std::vector<unsigned char>, unsigned int> DiskPackage::GetModelData(const std::string& fileName) const
{
    std::filesystem::path fileNamePath(fileName);

    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::MODELS_SUBDIR / fileNamePath.replace_extension() / fileName);
}

std::expected<Common::ImageData::Ptr, unsigned int> DiskPackage::GetTextureData(const std::string& fileName) const
{
    return GetTextureData(m_packageDir / Platform::ASSETS_DIR / Platform::TEXTURES_SUBDIR / fileName);
}

std::expected<Common::ImageData::Ptr, unsigned int>
DiskPackage::GetModelTextureData(const std::string& modelFileName, const std::string& textureFileName) const
{
    std::filesystem::path modelFileNamePath(modelFileName);

    return GetTextureData(m_packageDir / Platform::ASSETS_DIR / Platform::MODELS_SUBDIR / modelFileNamePath.replace_extension() / textureFileName);
}

}
