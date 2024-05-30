/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_DISKPACKAGE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_DISKPACKAGE_H

#include <Accela/Engine/Common.h>

#include <filesystem>
#include <expected>

namespace Accela::Engine
{
    struct DiskPackage
    {
        enum class CreateOnDiskError
        {
            DirectoryDoesntExist,
            PackageFileAlreadyExists,
            FailedToCreateDirectory,
            FailedToCreateSubdirectory,
            FailedToSerializeData,
            FailedToCreatePackageFile,
            FailedToCreateConstructFile
        };

        [[nodiscard]] static std::expected<std::filesystem::path, CreateOnDiskError> CreateOnDisk(
            const std::filesystem::path& dir,
            const PackageName& packageName);
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_DISKPACKAGE_H
