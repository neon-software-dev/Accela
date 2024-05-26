/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IPACKAGERESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IPACKAGERESOURCES_H

#include <Accela/Engine/Common.h>

#include <Accela/Platform/Package/Package.h>

#include <memory>
#include <future>
#include <optional>
#include <vector>

namespace Accela::Engine
{
    class IPackageResources
    {
        public:

            using Ptr = std::shared_ptr<IPackageResources>;

        public:

            virtual ~IPackageResources() = default;

            [[nodiscard]] virtual std::future<bool> OpenAndRegisterPackage(const PackageName& packageName) = 0;
            [[nodiscard]] virtual bool RegisterPackage(const Platform::Package::Ptr& package) = 0;
            [[nodiscard]] virtual std::vector<Platform::Package::Ptr> GetAllPackages() const = 0;
            [[nodiscard]] virtual std::optional<Platform::Package::Ptr> GetPackage(const PackageName& packageName) const = 0;
            virtual void ClosePackage(const PackageName& packageName) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IPACKAGERESOURCES_H
