/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FontResources.h"
#include "PackageResources.h"

#include <Accela/Platform/Text/IText.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

FontResources::FontResources(Common::ILogger::Ptr logger,
                             PackageResourcesPtr packages,
                             std::shared_ptr<Platform::IText> text,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
     : m_logger(std::move(logger))
     , m_packages(std::move(packages))
     , m_text(std::move(text))
     , m_threadPool(std::move(threadPool))
{

}

std::future<bool> FontResources::LoadFont(const PackageResourceIdentifier& resource, uint8_t fontSize)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadFont(resource, fontSize, fontSize)
        );
    });

    return messageFuture;
}

std::future<bool> FontResources::LoadFont(const PackageResourceIdentifier& resource, uint8_t startFontSize, uint8_t endFontSize)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadFont(resource, startFontSize, endFontSize)
        );
    });

    return messageFuture;
}

bool FontResources::OnLoadFont(const PackageResourceIdentifier& resource, uint8_t startFontSize, uint8_t endFontSize)
{
    const auto package = m_packages->GetPackageSource(*resource.GetPackageName());
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error,
          "FontResources::OnLoadFont: No such package: {}", resource.GetPackageName()->name);
        return false;
    }

    return LoadPackageFont(*package, resource, startFontSize, endFontSize);
}

std::future<bool> FontResources::LoadAllFonts(const PackageName& packageName, uint8_t startFontSize, uint8_t endFontSize)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAllFonts(packageName, startFontSize, endFontSize)
        );
    });

    return messageFuture;
}

bool FontResources::OnLoadAllFonts(const PackageName& packageName, uint8_t startFontSize, uint8_t endFontSize)
{
    m_logger->Log(Common::LogLevel::Info, "FontResources: Loading all fonts for package: {}", packageName.name);

    const auto package = m_packages->GetPackageSource(packageName);
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error,
          "FontResources::OnLoadAllFonts: No such package exists: {}", packageName.name);
        return false;
    }

    bool allSuccessful = true;

    for (const auto& fontResourceName : (*package)->GetFontResourceNames())
    {
        allSuccessful = allSuccessful && LoadPackageFont(
            *package,
            PRI((*package)->GetPackageName(), fontResourceName),
            startFontSize,
            endFontSize
        );
    }

    return allSuccessful;
}

std::future<bool> FontResources::LoadAllFonts(uint8_t startFontSize, uint8_t endFontSize)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAllFonts(startFontSize, endFontSize)
        );
    });

    return messageFuture;
}

bool FontResources::OnLoadAllFonts(uint8_t startFontSize, uint8_t endFontSize)
{
    m_logger->Log(Common::LogLevel::Info, "FontResources: Loading all fonts for all packages");

    bool allSuccessful = true;

    for (const auto& package : m_packages->GetAllPackages())
    {
        allSuccessful = allSuccessful && OnLoadAllFonts(PackageName(package->GetPackageName()), startFontSize, endFontSize);
    }

    return allSuccessful;
}

bool FontResources::LoadPackageFont(const Platform::PackageSource::Ptr& package,
                                    const PackageResourceIdentifier& resource,
                                    uint8_t startFontSize,
                                    uint8_t endFontSize)
{
    m_logger->Log(Common::LogLevel::Info,
      "FontResources: Loading package font: {} : {}-{}", resource.GetUniqueName(), startFontSize, endFontSize);

    const auto fontData = package->GetFontData(resource.GetResourceName());
    if (!fontData)
    {
        m_logger->Log(Common::LogLevel::Error,
          "FontResources::LoadPackageFont: Failed to get font data: {}", resource.GetUniqueName());
        return false;
    }

    bool allSuccessful = true;

    for (uint8_t fontSize = startFontSize; fontSize <= endFontSize; ++fontSize)
    {
        if (!m_text->LoadFontBlocking(resource.GetResourceName(), *fontData, fontSize)) { allSuccessful = false; }
    }

    return allSuccessful;
}

bool FontResources::IsFontLoaded(const ResourceIdentifier& resource, uint8_t fontSize)
{
    return m_text->IsFontLoaded(resource.GetResourceName(), fontSize);
}

void FontResources::DestroyFont(const ResourceIdentifier& resource)
{
    m_logger->Log(Common::LogLevel::Info, "FontResources: Destroying font resource: {}", resource.GetResourceName());

    m_text->UnloadFont(resource.GetResourceName());
}

void FontResources::DestroyFont(const ResourceIdentifier& resource, uint8_t fontSize)
{
    m_logger->Log(Common::LogLevel::Info,
      "FontResources: Destroying font resource: {} - {}", resource.GetResourceName(), fontSize);

    m_text->UnloadFont(resource.GetResourceName(), fontSize);
}

void FontResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "FontResources: Destroying all font resources");

    m_text->UnloadAllFonts();
}

}
