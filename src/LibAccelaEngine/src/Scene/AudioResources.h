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

#include <unordered_set>

namespace Accela::Engine
{
    class AudioResources : public IAudioResources
    {
        public:

            AudioResources(Common::ILogger::Ptr logger, AudioManagerPtr audioManager);

            //
            // IAudioResources
            //
            [[nodiscard]] bool RegisterAudio(const std::string& name, const Common::AudioData::Ptr& audioData) override;
            void DestroyAudio(const std::string& name) override;
            void DestroyAll() override;

        private:

            Common::ILogger::Ptr m_logger;
            AudioManagerPtr m_audioManager;

            std::unordered_set<std::string> m_audio;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_AUDIORESOURCES_H
