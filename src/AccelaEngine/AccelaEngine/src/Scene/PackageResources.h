/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_PACKAGERESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_PACKAGERESOURCES_H

#include "../ForwardDeclares.h"

#include <Accela/Engine/Scene/IPackageResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_map>

namespace Accela::Platform
{
    class IFiles;
}

namespace Accela::Engine
{
    class PackageResources : public IPackageResources
    {
        public:

            PackageResources(Common::ILogger::Ptr logger,
                             std::shared_ptr<Platform::IFiles> files,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IPackageResources
            //
            [[nodiscard]] std::future<bool> OpenAndRegisterPackage(const PackageName& packageName) override;
            [[nodiscard]] bool RegisterPackageSource(const Platform::PackageSource::Ptr& package) override;
            void ClosePackage(const PackageName& packageName) override;
            [[nodiscard]] std::future<std::expected<Construct::Ptr, bool>> FetchPackageConstruct(const PRI& construct) override;

            //
            // Internal
            //
            [[nodiscard]] std::vector<Platform::PackageSource::Ptr> GetAllPackages() const;
            [[nodiscard]] std::optional<Platform::PackageSource::Ptr> GetPackageSource(const PackageName& packageName) const;

        private:

            [[nodiscard]] bool OnOpenAndRegisterPackage(const PackageName& packageName);
            [[nodiscard]] std::expected<Construct::Ptr, bool> OnFetchPackageConstruct(const PRI& construct) const;

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_packagesMutex;
            std::unordered_map<PackageName, Platform::PackageSource::Ptr> m_packages;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_PACKAGERESOURCES_H
