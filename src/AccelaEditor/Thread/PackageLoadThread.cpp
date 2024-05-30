/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PackageLoadThread.h"

#include "../EditorScene/Messages.h"

#include <Accela/Engine/Package/Package.h>
#include <Accela/Engine/Package/Construct.h>
#include <Accela/Platform/Package/DiskPackageSource.h>

namespace Accela
{

PackageLoadThread::PackageLoadThread(QObject *pParent, AccelaWindow* pAccelaWindow, std::filesystem::path packageFilePath)
    : QThread(pParent)
    , m_pAccelaWindow(pAccelaWindow)
    , m_packageFilePath(std::move(packageFilePath))
{

}

void PackageLoadThread::run()
{
    emit ProgressUpdate(0, 4, "Opening Package");

    //
    // Open the package on disk; just loads metadata about what files exist in the package into
    // a DiskPackageSource, which then can be used to fetch individual file data.
    //
    const auto packageSource = Platform::DiskPackageSource::OpenOnDisk(m_packageFilePath);
    if (!packageSource)
    {
        emit PackageLoadFinished(std::unexpected(0x1000 + (unsigned int)packageSource.error()));
        return;
    }

    const std::shared_ptr<Platform::DiskPackageSource> diskPackageSource =
        std::dynamic_pointer_cast<Platform::DiskPackageSource>(*packageSource);

    const unsigned int totalNumLoadSteps =
        1 + // Package open
        1 + // Package file load
        diskPackageSource->GetConstructResourceNames().size() + // Construct file loads
        1; // Resources load

    unsigned int curStep = 0;

    //
    // Load Package File
    //
    const auto packageFilePath = diskPackageSource->GetPackageFilePath();

    emit ProgressUpdate(++curStep, totalNumLoadSteps, std::format("Loading {}", packageFilePath.filename().string()));

    const auto packageFileData = diskPackageSource->GetPackageFileData();
    if (!packageFileData)
    {
        emit PackageLoadFinished(std::unexpected(0x2000 + packageFileData.error()));
        return;
    }

    const auto package = Engine::Package::FromBytes(diskPackageSource->GetPackageName(), *packageFileData);
    if (!package)
    {
        emit PackageLoadFinished(std::unexpected(0x3000 + packageFileData.error()));
        return;
    }

    //
    // Constructs
    //
    std::vector<Engine::Construct::Ptr> constructs;

    for (const auto& resourceName : diskPackageSource->GetConstructResourceNames())
    {
        emit ProgressUpdate(++curStep, totalNumLoadSteps, std::format("Loading {}", resourceName));

        const auto constructData = diskPackageSource->GetConstructData(resourceName);
        if (!constructData)
        {
            emit PackageLoadFinished(std::unexpected(0x4000 + constructData.error()));
            return;
        }

        const auto construct = Engine::Construct::FromBytes(resourceName, *constructData);
        if (!construct)
        {
            emit PackageLoadFinished(std::unexpected(0x5000 + construct.error()));
            return;
        }

        constructs.push_back(*construct);
    }

    //
    // Resources
    //
    emit ProgressUpdate(++curStep, totalNumLoadSteps, "Loading Resources");

    // Message the scene to request it to load the package's resources
    auto msg = std::make_shared<LoadPackageResourcesCommand>(
        Engine::PackageName((*package)->GetName())
    );
    auto fut = msg->CreateFuture();

    m_pAccelaWindow->EnqueueSceneMessage(msg);

    // Block this package load thread until Accela has finished loading the resources
    const bool loadResult = fut.get();
    if (!loadResult)
    {
        emit PackageLoadFinished(std::unexpected(0x6000));
        return;
    }

    emit PackageLoadFinished(diskPackageSource);
}

}
