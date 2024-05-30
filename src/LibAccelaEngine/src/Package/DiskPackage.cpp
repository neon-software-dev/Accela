/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Package/DiskPackage.h>
#include <Accela/Engine/Package/Package.h>
#include <Accela/Engine/Package/Construct.h>

#include <Accela/Platform/File/IFiles.h>

#include <nlohmann/json.hpp>

#include <fstream>

namespace Accela::Engine
{

std::expected<std::filesystem::path, DiskPackage::CreateOnDiskError> DiskPackage::CreateOnDisk(
    const std::filesystem::path& dir,
    const PackageName& packageName)
{
    std::error_code ec{};

    // PackageSource Directory (e.g. '/path/to/packages/PackageName')
    const auto packageDir = dir / std::filesystem::path(packageName.name);

    // FileName of the package file (e.g. 'PackageName.acp')
    const auto packageFileName = packageName.name + Platform::PACKAGE_EXTENSION;

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

        const auto defaultPackage = Package(packageName.name, PACKAGE_VERSION);
        const auto defaultPackageBytes = defaultPackage.ToBytes();
        if (!defaultPackageBytes)
        {
            return std::unexpected(CreateOnDiskError::FailedToSerializeData);
        }

        ofs.write(reinterpret_cast<const char*>(defaultPackageBytes->data()), (long)defaultPackageBytes->size());
        ofs.close();
    }

    // Create a default construct
    {
        const auto defaultConstructName = "default";

        const auto defaultConstructPath =
            packageDir / Platform::CONSTRUCTS_DIR / (defaultConstructName + std::string(Platform::CONSTRUCT_EXTENSION));

        std::ofstream ofs;
        ofs.open(defaultConstructPath, std::ofstream::out);

        if (!ofs.good())
        {
            return std::unexpected(CreateOnDiskError::FailedToCreateConstructFile);
        }

        const auto defaultConstruct = Construct(defaultConstructName);
        const auto defaultConstructBytes = defaultConstruct.ToBytes();
        if (!defaultConstructBytes)
        {
            return std::unexpected(CreateOnDiskError::FailedToSerializeData);
        }

        ofs.write(reinterpret_cast<const char*>(defaultConstructBytes->data()), (long)defaultConstructBytes->size());
        ofs.close();
    }

    return packageFilePath;
}

}
