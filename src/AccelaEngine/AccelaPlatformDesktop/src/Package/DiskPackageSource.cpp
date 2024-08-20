/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/Package/DiskPackageSource.h>

#include <Accela/Platform/File/IFiles.h>

#include <nlohmann/json.hpp>

#include <fstream>

namespace Accela::Platform
{

std::expected<PackageSource::Ptr, DiskPackageSource::OpenPackageError> DiskPackageSource::OpenOnDisk(const std::filesystem::path& manifestFile)
{
    //
    // The manifest file should exist
    //
    if (!std::filesystem::exists(manifestFile))
    {
        return std::unexpected(OpenPackageError::PackageFileDoesntExist);
    }

    //
    // Subdirectories for assets and constructs should exist
    //
    const auto manifestDir = manifestFile.parent_path();

    if (!std::filesystem::exists(manifestDir / Platform::ASSETS_DIR))
    {
        return std::unexpected(OpenPackageError::PackageStructureBroken);
    }
    if (!std::filesystem::exists(manifestDir / Platform::CONSTRUCTS_DIR))
    {
        return std::unexpected(OpenPackageError::PackageStructureBroken);
    }

    //
    // Create the package source object and have it load its metadata from the package
    //
    auto packageSource = std::make_shared<DiskPackageSource>(Tag{}, manifestDir, manifestFile);

    if (!packageSource->LoadMetadata())
    {
        return std::unexpected(OpenPackageError::FailureLoadingMetadata);
    }

    return packageSource;
}

DiskPackageSource::DiskPackageSource(DiskPackageSource::Tag, std::filesystem::path packageDir, std::filesystem::path packageFilePath)
    : PackageSource(packageFilePath.filename().replace_extension().string())
    , m_packageDir(std::move(packageDir))
    , m_manifestFilePath(std::move(packageFilePath))
{

}

std::vector<std::string> DiskPackageSource::GetAudioResourceNames() const
{
    return GetFileNames(m_audioAssets);
}

std::vector<std::string> DiskPackageSource::GetFontResourceNames() const
{
    return GetFileNames(m_fontAssets);
}

std::vector<std::string> DiskPackageSource::GetModelResourceNames() const
{
    return GetFileNames(m_modelAssets);
}

std::vector<std::string> DiskPackageSource::GetTextureResourceNames() const
{
    return GetFileNames(m_textureAssets);
}

std::vector<std::string> DiskPackageSource::GetConstructResourceNames() const
{
    return GetFileNames(m_constructs);
}

bool DiskPackageSource::LoadMetadata()
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

std::expected<std::vector<std::filesystem::path>, bool> DiskPackageSource::GetFilePaths(const std::filesystem::path& directory)
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

std::expected<std::vector<std::filesystem::path>, bool> DiskPackageSource::GetModelFilePaths(const std::filesystem::path& directory)
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

std::vector<std::string> DiskPackageSource::GetFileNames(const std::vector<std::filesystem::path>& filePaths)
{
    std::vector<std::string> names;

    std::ranges::transform(filePaths, std::back_inserter(names), [](const auto& filePath){
        return filePath.filename().string();
    });

    return names;
}

std::expected<std::vector<std::byte>, bool> DiskPackageSource::GetFileBytes(const std::filesystem::path& filePath)
{
    std::error_code ec{};

    if (!std::filesystem::exists(filePath, ec) || ec)
    {
        return std::unexpected(0);
    }

    const auto fileSize = std::filesystem::file_size(filePath, ec);
    if (ec)
    {
        return std::unexpected(false);
    }

    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected(false);
    }

    std::vector<std::byte> fileContents(fileSize);
    file.read(reinterpret_cast<char*>(fileContents.data()), (long)fileSize);

    return fileContents;
}

std::expected<std::string, bool> DiskPackageSource::GetTextureFormatHint(const std::string& resourceName) const
{
    const auto path = std::filesystem::path(resourceName);
    if (!path.has_extension())
    {
        return std::unexpected(false);
    }

    std::string hint = path.extension().string();

    // Remove leading period from the extension name (.jpg - > jpg)
    hint = hint.substr(1, hint.size() - 1);

    return hint;
}

std::expected<std::string, bool>
DiskPackageSource::GetModelTextureFormatHint(const std::string&, const std::string& resourceName) const
{
    const auto path = std::filesystem::path(resourceName);
    if (!path.has_extension())
    {
        return std::unexpected(false);
    }

    return path.extension().string();
}

std::expected<std::vector<std::byte>, unsigned int> DiskPackageSource::GetManifestFileData() const
{
    return GetFileBytes(m_manifestFilePath);
}

std::expected<std::vector<std::byte>, unsigned int> DiskPackageSource::GetFontData(const std::string& resourceName) const
{
    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::FONTS_SUBDIR / resourceName);
}

std::expected<std::vector<std::byte>, unsigned int> DiskPackageSource::GetAudioData(const std::string& resourceName) const
{
    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::AUDIO_SUBDIR / resourceName);
}

std::expected<std::vector<std::byte>, unsigned int> DiskPackageSource::GetModelData(const std::string& resourceName) const
{
    std::filesystem::path fileNamePath(resourceName);

    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::MODELS_SUBDIR / fileNamePath.replace_extension() / resourceName);
}

std::expected<std::vector<std::byte>, unsigned int> DiskPackageSource::GetTextureData(const std::string& resourceName) const
{
    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::TEXTURES_SUBDIR / resourceName);
}

std::expected<std::vector<std::byte>, unsigned int>
DiskPackageSource::GetModelTextureData(const std::string& modelResourceName, const std::string& textureResourceName) const
{
    std::filesystem::path modelFileNamePath(modelResourceName);

    return GetFileBytes(m_packageDir / Platform::ASSETS_DIR / Platform::MODELS_SUBDIR / modelFileNamePath.replace_extension() / textureResourceName);
}

std::expected<std::vector<std::byte>, unsigned int>
DiskPackageSource::GetConstructData(const std::string& constructName) const
{
    return GetFileBytes(m_packageDir / Platform::CONSTRUCTS_DIR / constructName);
}

}
