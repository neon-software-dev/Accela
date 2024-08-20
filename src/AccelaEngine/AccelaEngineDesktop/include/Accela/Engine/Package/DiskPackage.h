/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_PACKAGE_DISKPACKAGE_H
#define LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_PACKAGE_DISKPACKAGE_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Package/Package.h>

#include <Accela/Common/SharedLib.h>

#include <filesystem>
#include <expected>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC DiskPackage
    {
        enum class CreateOnDiskError
        {
            DirectoryDoesntExist,
            PackageFileAlreadyExists,
            FailedToCreateDirectory,
            FailedToCreateSubdirectory,
            FailedToWriteFiles
        };

        /**
         * Creates a default package + directory structure.
         *
         * Will create a directory named packageName within dir, with sub-directories and
         * default files within it, as appropriate. Will error out if this directory already
         * exists.
         *
         * @param dir The root directory to create the package within
         * @param packageName The name of the package to create
         *
         * @return The path to the manifest file on success, or CreateOnDiskError on error
         */
        [[nodiscard]] static std::expected<std::filesystem::path, CreateOnDiskError> CreateOnDisk(
            const std::filesystem::path& dir,
            const PackageName& packageName);

        /**
         * Write the files (manifest/constructs) for a package to a given package dir.
         *
         * @param packageDir The root package directory to write the files to
         * @param package The package to be written.
         *
         * @return Whether the operation was successful
         */
        [[nodiscard]] static bool WritePackageFilesToDisk(const std::filesystem::path& packageDir,
                                                          const Package& package);
    };
}

#endif //LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_PACKAGE_DISKPACKAGE_H
