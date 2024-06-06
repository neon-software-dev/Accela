/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PackageLoadThread.h"

#include "../EditorScene/SceneSyncer.h"

#include <Accela/Engine/Package/Manifest.h>

namespace Accela
{

PackageLoadThread::PackageLoadThread(QObject *pParent, SceneSyncer* pSceneSyncer, std::filesystem::path packageFilePath)
    : QThread(pParent)
    , m_pSceneSyncer(pSceneSyncer)
    , m_packageFilePath(std::move(packageFilePath))
{

}

void PackageLoadThread::run()
{
    Engine::Package result{};

    std::vector<Step> steps;

    //
    // Run an initial step (with a guessed total number of steps) which opens the package
    // into a DiskPackageSource, through which we can then determine all the additional load
    // steps that need to be performed.
    //
    steps.push_back(OpenDiskPackageStep(result.source));

    if (!RunSteps(0, 6, steps))
    {
        return;
    }

    const auto diskPackageSource = std::dynamic_pointer_cast<Platform::DiskPackageSource>(result.source);

    //

    steps.clear();

    const auto constructResourceNames = diskPackageSource->GetConstructResourceNames();

    const unsigned int totalNumSteps =
        1 +                             // Package open
        1 +                             // Destroy previous entities
        1 +                             // Destroy previous resources
        1 +                             // Package file load
        constructResourceNames.size() + // Construct file loads
        1;                              // Resources load

    // 2 - Step to destroy any previously created entities
    steps.push_back(DestroyEntitiesStep());

    // 3 - Step to destroy any previously loaded resources
    steps.push_back(DestroyResourcesStep());

    // 4 - Step to load the manifest file into a Manifest object
    steps.push_back(LoadManifestFileStep(diskPackageSource, result.manifest));

    // 5 - Step(s) to load the construct files into Construct objects
    result.constructs.resize(constructResourceNames.size());

    for (unsigned int x = 0; x < constructResourceNames.size(); ++x)
    {
        steps.push_back(LoadConstructFileStep(diskPackageSource,
                                              constructResourceNames[x],
                                              result.constructs[x]));
    }

    // 6 - Step to load package resources into the engine
    steps.push_back(LoadPackageResourcesStep(result.manifest));

    if (!RunSteps(1, totalNumSteps, steps))
    {
        return;
    }

    if (!m_isCancelled)
    {
        emit PackageLoadFinished(result);
    }
}

bool PackageLoadThread::RunSteps(unsigned int numStepsRunBefore, unsigned int numTotalSteps, const std::vector<Step>& steps)
{
    unsigned int stepIndex = numStepsRunBefore;

    for (const auto& step : steps)
    {
        if (m_isCancelled)
        {
            return false;
        }

        if (!RunStep(step, stepIndex++, numTotalSteps))
        {
            return false;
        }
    }

    return true;
}

bool PackageLoadThread::RunStep(const PackageLoadThread::Step& step, unsigned int stepIndex, unsigned int totalSteps)
{
    emit ProgressUpdate(stepIndex, totalSteps, step.status);

    const auto result = std::invoke(step.logic);
    if (result != 0)
    {
        if (!m_isCancelled)
        {
            emit PackageLoadFinished(std::unexpected((stepIndex * 1000) + result));
        }

        return false;
    }

    return true;
}

std::expected<Platform::DiskPackageSource::Ptr, unsigned int> PackageLoadThread::OpenDiskPackage(const std::filesystem::path& packageFilePath)
{
    const auto packageSource = Platform::DiskPackageSource::OpenOnDisk(packageFilePath);
    if (!packageSource)
    {
        return std::unexpected(1);
    }

    return std::dynamic_pointer_cast<Platform::DiskPackageSource>(*packageSource);
}

std::expected<Engine::Manifest, unsigned int> PackageLoadThread::LoadManifestFile(const Platform::DiskPackageSource::Ptr& diskPackageSource)
{
    const auto packageFilePath = diskPackageSource->GetManifestFilePath();

    const auto packageFileData = diskPackageSource->GetManifestFileData();
    if (!packageFileData)
    {
        return std::unexpected(1);
    }

    const auto manifest = Engine::Manifest::FromBytes(*packageFileData);
    if (!manifest)
    {
        return std::unexpected(2);
    }

    return *manifest;
}

std::expected<Engine::Construct::Ptr, unsigned int> PackageLoadThread::LoadConstructFile(const Platform::DiskPackageSource::Ptr& diskPackageSource,
                                                                                         const std::string& constructResourceName)
{
    const auto constructData = diskPackageSource->GetConstructData(constructResourceName);
    if (!constructData)
    {
        return std::unexpected(1);
    }

    const auto construct = Engine::Construct::FromBytes(*constructData);
    if (!construct)
    {
        return std::unexpected(2);
    }

    return *construct;
}

unsigned int PackageLoadThread::LoadPackageResources(const Engine::Manifest& manifest) const
{
    const auto packageName = Engine::PackageName(manifest.GetPackageName());

    if (!m_pSceneSyncer->LoadPackageResources(packageName).get())
    {
        return 1;
    }

    return 0;
}

PackageLoadThread::Step PackageLoadThread::DestroyEntitiesStep() const
{
    return {
        "Destroying Previous Entities",
        [&](){
            if (!m_pSceneSyncer->DestroyAllEntities().get())
            {
                return 1U;
            }
            return 0U;
        }
    };
}

PackageLoadThread::Step PackageLoadThread::DestroyResourcesStep() const
{
    return {
        "Destroying Previous Resources",
        [&](){
            if (!m_pSceneSyncer->DestroyAllResources().get())
            {
                return 1U;
            }
            return 0U;
        }
    };
}

PackageLoadThread::Step PackageLoadThread::OpenDiskPackageStep(Platform::PackageSource::Ptr& out) const
{
    return {
        "Opening Package",
        [&](){
            const auto result = OpenDiskPackage(m_packageFilePath);
            if (!result) { return result.error(); }
            out = *result;
            return 0U;
        }
    };
}

PackageLoadThread::Step PackageLoadThread::LoadManifestFileStep(const Platform::DiskPackageSource::Ptr& diskPackageSource,
                                                                Engine::Manifest& out) const
{
    return {
        std::format("Loading {}", diskPackageSource->GetPackageName()),
        [&](){
            const auto result = LoadManifestFile(diskPackageSource);
            if (!result) { return result.error(); }
            out = *result;
            return 0U;
        }
    };
}

PackageLoadThread::Step PackageLoadThread::LoadConstructFileStep(const Platform::DiskPackageSource::Ptr& diskPackageSource,
                                                                 const std::string& constructResourceName,
                                                                 Engine::Construct::Ptr& out) const
{
    return {
        std::format("Loading {}", constructResourceName),
        [&](){
            const auto result = LoadConstructFile(diskPackageSource, constructResourceName);
            if (!result) { return result.error(); }
            out = *result;
            return 0U;
        }
    };
}

PackageLoadThread::Step PackageLoadThread::LoadPackageResourcesStep(const Engine::Manifest& manifest) const
{
    return {
        std::format("Loading Resources"),
        [&](){
            const auto result = LoadPackageResources(manifest);
            if (result != 0) { return result; }
            return 0U;
        }
    };
}

}
