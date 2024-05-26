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
            [[nodiscard]] bool RegisterPackage(const Platform::Package::Ptr& package) override;
            [[nodiscard]] std::vector<Platform::Package::Ptr> GetAllPackages() const override;
            [[nodiscard]] std::optional<Platform::Package::Ptr> GetPackage(const PackageName& packageName) const override;
            void ClosePackage(const PackageName& packageName) override;

        private:

            [[nodiscard]] bool OnOpenAndRegisterPackage(const PackageName& packageName);

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_packagesMutex;
            std::unordered_map<PackageName, Platform::Package::Ptr> m_packages;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_PACKAGERESOURCES_H
