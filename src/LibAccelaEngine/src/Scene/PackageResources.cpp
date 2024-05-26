/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PackageResources.h"

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

PackageResources::PackageResources(Common::ILogger::Ptr logger,
                                   std::shared_ptr<Platform::IFiles> files,
                                   std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_files(std::move(files))
    , m_threadPool(std::move(threadPool))
{

}

std::future<bool> PackageResources::OpenAndRegisterPackage(const PackageName& packageName)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnOpenAndRegisterPackage(packageName)
        );
    });

    return messageFuture;
}

bool PackageResources::RegisterPackage(const Platform::Package::Ptr& package)
{
    const auto packageName = PackageName(package->GetPackageName());

    m_logger->Log(Common::LogLevel::Info, "PackageResources: Registering package: {}", packageName.name);

    std::lock_guard<std::mutex> lock(m_packagesMutex);

    const auto it = m_packages.find(packageName);
    if (it != m_packages.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "PackageResources::RegisterPackage: Package already registered, ignoring: {}", packageName.name);
        return true;
    }

    m_packages.insert({packageName, package});

    return true;
}

std::vector<Platform::Package::Ptr> PackageResources::GetAllPackages() const
{
    std::lock_guard<std::mutex> lock(m_packagesMutex);

    std::vector<Platform::Package::Ptr> packages;

    for (const auto& it : m_packages)
    {
        packages.push_back(it.second);
    }

    return packages;
}

std::optional<Platform::Package::Ptr> PackageResources::GetPackage(const PackageName& packageName) const
{
    std::lock_guard<std::mutex> lock(m_packagesMutex);

    const auto it = m_packages.find(packageName);
    if (it == m_packages.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void PackageResources::ClosePackage(const PackageName& packageName)
{
    m_logger->Log(Common::LogLevel::Info, "PackageResources: Closing package: {}", packageName.name);

    std::lock_guard<std::mutex> lock(m_packagesMutex);

    m_packages.erase(packageName);
}

bool PackageResources::OnOpenAndRegisterPackage(const PackageName& packageName)
{
    m_logger->Log(Common::LogLevel::Info, "PackageResources:: Opening package: {}", packageName.name);

    const auto package = m_files->LoadPackage(packageName.name);
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error, "PackageResources::OnOpenAndRegisterPackage: Package load failed");
        return false;
    }

    return RegisterPackage(*package);
}

}
