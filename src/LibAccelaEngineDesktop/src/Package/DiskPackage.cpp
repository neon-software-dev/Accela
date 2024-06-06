/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Package/DiskPackage.h>
#include <Accela/Engine/Package/Manifest.h>
#include <Accela/Engine/Package/Construct.h>

#include <Accela/Platform/File/IFiles.h>

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

    //
    // Create a default package and write it to disk
    //
    Package package(
        nullptr,
        Manifest(packageName.name, MANIFEST_VERSION),
        {std::make_shared<Construct>("default")}
    );

    if (!WritePackageFilesToDisk(packageDir, package))
    {
        return std::unexpected(CreateOnDiskError::FailedToWriteFiles);
    }

    return packageFilePath;
}

bool DiskPackage::WritePackageFilesToDisk(const std::filesystem::path& packageDir, const Package& package)
{
    std::error_code ec{};

    // If the package directory doesn't exist, bail out
    if (!std::filesystem::exists(packageDir))
    {
        return false;
    }

    //
    // Write the manifest file
    //
    {
        // FileName of the package file (e.g. 'PackageName.acp')
        const auto packageFileName = package.manifest.GetPackageName() + Platform::PACKAGE_EXTENSION;

        // Full path to the package file on disk
        auto packageFilePath = packageDir / packageFileName;

        std::ofstream ofs;
        ofs.open(packageFilePath, std::ofstream::out | std::ofstream::trunc);

        if (!ofs.good())
        {
            return false;
        }

        const auto manifestBytes = package.manifest.ToBytes();
        if (!manifestBytes)
        {
            return false;
        }

        ofs.write(reinterpret_cast<const char *>(manifestBytes->data()), (long) manifestBytes->size());
        ofs.close();
    }

    //
    // Write the construct files
    //
    for (const auto& construct : package.constructs)
    {
        const auto constructPath =
            packageDir / Platform::CONSTRUCTS_DIR / (construct->GetName() + std::string(Platform::CONSTRUCT_EXTENSION));

        std::ofstream ofs;
        ofs.open(constructPath, std::ofstream::out | std::ofstream::trunc);

        if (!ofs.good())
        {
            return false;
        }

        const auto constructBytes = construct->ToBytes();
        if (!constructBytes)
        {
            return false;
        }

        ofs.write(reinterpret_cast<const char *>(constructBytes->data()), (long) constructBytes->size());
        ofs.close();
    }

    return true;
}

}
