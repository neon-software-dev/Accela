/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_THREAD_PACKAGELOADTHREAD_H
#define ACCELAEDITOR_THREAD_PACKAGELOADTHREAD_H

#include <Accela/Engine/Package/Package.h>

#include <Accela/Platform/Package/DiskPackageSource.h>

#include <QThread>

#include <memory>
#include <filesystem>
#include <expected>
#include <string>
#include <algorithm>
#include <vector>

namespace Accela
{
    class SceneSyncer;

    namespace Engine
    {
        class IPackageResources;
    }

    class PackageLoadThread : public QThread
    {
        Q_OBJECT

        public:

            PackageLoadThread(QObject* pParent, SceneSyncer* pSceneSyncer, std::filesystem::path packageFilePath);

        signals:

            void ProgressUpdate(unsigned int progress, unsigned int total, const std::string& progressText);
            void PackageLoadFinished(const std::expected<Engine::Package, unsigned int>& result);

        public slots:

            void Cancel() { m_isCancelled = true; }

        protected:

            void run() override;

        private:

            struct Step
            {
                Step(std::string _status, std::function<unsigned int()> _logic)
                    : status(std::move(_status))
                    , logic(std::move(_logic))
                { }

                std::string status;
                std::function<unsigned int()> logic;
            };

        private:

            [[nodiscard]] bool RunSteps(unsigned int numStepsRunBefore, unsigned int numTotalSteps, const std::vector<Step>& steps);
            [[nodiscard]] bool RunStep(const Step& step, unsigned int stepIndex, unsigned int totalSteps);

            [[nodiscard]] static std::expected<Platform::DiskPackageSource::Ptr, unsigned int> OpenDiskPackage(
                const std::filesystem::path& packageFilePath);
            [[nodiscard]] static std::expected<Engine::Manifest, unsigned int> LoadManifestFile(
                const Platform::DiskPackageSource::Ptr& diskPackageSource);
            [[nodiscard]] static std::expected<Engine::Construct::Ptr, unsigned int> LoadConstructFile(
                const Platform::DiskPackageSource::Ptr& diskPackageSource,
                const std::string& constructResourceName);
            [[nodiscard]] unsigned int LoadPackageResources(const Engine::Manifest& manifest) const;

            [[nodiscard]] Step DestroyEntitiesStep() const;
            [[nodiscard]] Step DestroyResourcesStep() const;
            [[nodiscard]] Step OpenDiskPackageStep(Platform::PackageSource::Ptr& out) const;
            [[nodiscard]] Step LoadManifestFileStep(const Platform::DiskPackageSource::Ptr& diskPackageSource, Engine::Manifest& out) const;
            [[nodiscard]] Step LoadConstructFileStep(const Platform::DiskPackageSource::Ptr& diskPackageSource,
                                                     const std::string& constructResourceName,
                                                     Engine::Construct::Ptr& out) const;
            [[nodiscard]] Step LoadPackageResourcesStep(const Engine::Manifest& manifest) const;

        private:

            SceneSyncer* m_pSceneSyncer;

            std::filesystem::path m_packageFilePath;

            bool m_isCancelled{false};
    };
}

#endif //ACCELAEDITOR_THREAD_PACKAGELOADTHREAD_H
