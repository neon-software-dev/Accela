/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H

#include "../ForwardDeclares.h"

#include <Accela/Engine/Scene/IAudioResources.h>

#include "Accela/Platform/Package/PackageSource.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_set>
#include <unordered_map>
#include <expected>

namespace Accela::Platform
{
    class IFiles;
}

namespace Accela::Engine
{
    class AudioResources : public IAudioResources
    {
        public:

            AudioResources(Common::ILogger::Ptr logger,
                           PackageResourcesPtr packages,
                           AudioManagerPtr audioManager,
                           std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IAudioResources
            //
            [[nodiscard]] std::future<bool> LoadAudio(const PackageResourceIdentifier& resource) override;
            [[nodiscard]] bool LoadAudio(const CustomResourceIdentifier& resource, const Common::AudioData::Ptr& audioData) override;
            [[nodiscard]] std::future<bool> LoadAllAudio(const PackageName& packageName) override;
            [[nodiscard]] std::future<bool> LoadAllAudio() override;
            void DestroyAudio(const ResourceIdentifier& resource) override;
            void DestroyAllAudio(const PackageName& packageName) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] bool OnLoadAudio(const PackageResourceIdentifier& resource);
            [[nodiscard]] bool OnLoadAllAudio(const PackageName& packageName);
            [[nodiscard]] bool OnLoadAllAudio();

            [[nodiscard]] bool LoadPackageAudio(const Platform::PackageSource::Ptr& package, const PackageResourceIdentifier& resource);

            [[nodiscard]] std::expected<Common::AudioData::Ptr, bool> AudioDataFromBytes(std::vector<std::byte>& bytes, const std::string& tag) const;

        private:

            Common::ILogger::Ptr m_logger;
            PackageResourcesPtr m_packages;
            AudioManagerPtr m_audioManager;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            std::mutex m_audioMutex;
            std::unordered_map<PackageName, std::unordered_set<ResourceIdentifier>> m_packageAudio;
            std::unordered_set<ResourceIdentifier> m_customAudio;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H
