/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H

#include "../ForwardDeclares.h"

#include <Accela/Engine/Scene/IAudioResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <unordered_set>

namespace Accela::Platform
{
    class IFiles;
}

namespace Accela::Engine
{
    class IEngineAssets;

    class AudioResources : public IAudioResources
    {
        public:

            AudioResources(Common::ILogger::Ptr logger,
                           std::shared_ptr<IEngineAssets> assets,
                           std::shared_ptr<Platform::IFiles> files,
                           AudioManagerPtr audioManager,
                           std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IAudioResources
            //
            [[nodiscard]] std::future<bool> LoadAssetsAudio(const std::string& audioFileName) override;
            [[nodiscard]] std::future<bool> LoadAllAssetAudio() override;
            [[nodiscard]] bool LoadAudio(const std::string& name, const Common::AudioData::Ptr& audioData) override;
            void DestroyAudio(const std::string& name) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] bool OnLoadAssetsAudio(const std::string& audioFileName);
            [[nodiscard]] bool OnLoadAllAssetAudio();

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Platform::IFiles> m_files;
            AudioManagerPtr m_audioManager;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            std::mutex m_audioMutex;
            std::unordered_set<std::string> m_audio;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H
