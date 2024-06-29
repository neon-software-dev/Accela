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

struct FetchConstructResultMessage : public Common::ResultMessage<std::expected<Construct::Ptr, bool>>
{
        FetchConstructResultMessage()
        : Common::ResultMessage<std::expected<Construct::Ptr, bool>>("FetchConstructResultMessage")
    { }
};

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

bool PackageResources::RegisterPackageSource(const Platform::PackageSource::Ptr& package)
{
    const auto packageName = PackageName(package->GetPackageName());

    m_logger->Log(Common::LogLevel::Info, "PackageResources: Registering package: {}", packageName.name);

    std::lock_guard<std::mutex> lock(m_packagesMutex);

    const auto it = m_packages.find(packageName);
    if (it != m_packages.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "PackageResources::RegisterPackage: PackageSource already registered, ignoring: {}", packageName.name);
        return true;
    }

    m_packages.insert({packageName, package});

    return true;
}

std::vector<Platform::PackageSource::Ptr> PackageResources::GetAllPackages() const
{
    std::lock_guard<std::mutex> lock(m_packagesMutex);

    std::vector<Platform::PackageSource::Ptr> packages;

    for (const auto& it : m_packages)
    {
        packages.push_back(it.second);
    }

    return packages;
}

std::optional<Platform::PackageSource::Ptr> PackageResources::GetPackageSource(const PackageName& packageName) const
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

std::future<std::expected<Construct::Ptr, bool>> PackageResources::FetchPackageConstruct(const PRI& construct)
{
    auto message = std::make_shared<FetchConstructResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<FetchConstructResultMessage>(_message)->SetResult(
            OnFetchPackageConstruct(construct)
        );
    });

    return messageFuture;
}

bool PackageResources::OnOpenAndRegisterPackage(const PackageName& packageName)
{
    m_logger->Log(Common::LogLevel::Info, "PackageResources:: Opening package: {}", packageName.name);

    const auto package = m_files->LoadPackage(packageName.name);
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error, "PackageResources::OnOpenAndRegisterPackage: PackageSource load failed");
        return false;
    }

    return RegisterPackageSource(*package);
}

std::expected<Construct::Ptr, bool> PackageResources::OnFetchPackageConstruct(const PRI& construct) const
{
    const auto package = m_packages.find(*construct.GetPackageName());
    if (package == m_packages.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PackageResources::OnFetchPackageConstruct: No such package is registered: {}", construct.GetPackageName()->name);
        return std::unexpected(false);
    }

    const auto constructData = package->second->GetConstructData(construct.GetResourceName());
    if (!constructData)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PackageResources::OnFetchPackageConstruct: Failed to get construct data from package: {}", construct.GetResourceName());
        return std::unexpected(false);
    }

    const auto constructObj = Construct::FromBytes(*constructData);
    if (!constructObj)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PackageResources::OnFetchPackageConstruct: Failed to create construct from bytes: {}", construct.GetResourceName());
        return std::unexpected(false);
    }

    return *constructObj;
}

}
