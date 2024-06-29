/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IPACKAGERESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IPACKAGERESOURCES_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/ResourceIdentifier.h>
#include <Accela/Engine/Package/Construct.h>

#include <Accela/Platform/Package/PackageSource.h>

#include <memory>
#include <future>
#include <vector>
#include <expected>

namespace Accela::Engine
{
    class IPackageResources
    {
        public:

            using Ptr = std::shared_ptr<IPackageResources>;

        public:

            virtual ~IPackageResources() = default;

            /**
             * Registers a package from the accela packages directory. Loads and processes the package file,
             * but doesn't load any of its resources or constructs from disk.
             *
             * @param packageName The name of the package
             *
             * @return Whether the package was registered successfully
             */
            [[nodiscard]] virtual std::future<bool> OpenAndRegisterPackage(const PackageName& packageName) = 0;

            /**
             * Registers a package from a client-provided package source
             *
             * @param package The PackageSource implementation that provides the package's data
             *
             * @return Whether the package was registered successfully
             */
            [[nodiscard]] virtual bool RegisterPackageSource(const Platform::PackageSource::Ptr& package) = 0;

            /**
             * Close a previously registered package
             */
            virtual void ClosePackage(const PackageName& packageName) = 0;

            /**
             * Fetches a particular Construct's data from a previously registered Package
             *
             * @param construct Identifies the package/construct to be fetched
             *
             * @return The Construct object, or false on error
             */
            [[nodiscard]] virtual std::future<std::expected<Construct::Ptr, bool>> FetchPackageConstruct(const PRI& construct) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IPACKAGERESOURCES_H
