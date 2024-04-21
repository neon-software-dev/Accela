/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_SRC_AUDIO_AUDIOMANAGER_H
#define LIBACCELAENGINE_SRC_AUDIO_AUDIOMANAGER_H

#include <Accela/Engine/Audio/AudioCommon.h>
#include <Accela/Engine/Audio/AudioSourceProperties.h>
#include <Accela/Engine/Audio/AudioListener.h>

#include <Accela/Common/AudioData.h>
#include <Accela/Common/Log/ILogger.h>

#include <alext.h>

#include <glm/glm.hpp>

#include <expected>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace Accela::Engine
{
    class AudioManager
    {
        public:

            using Ptr = std::shared_ptr<AudioManager>;

            struct SourceProperties
            {
                bool localSource{false};
                AudioSourceProperties audioProperties;
            };

        public:

            explicit AudioManager(Common::ILogger::Ptr logger);

            bool Startup();
            void Shutdown();

            [[nodiscard]] bool RegisterAudio(const std::string& name, const Common::AudioData::Ptr& audioData);
            void DestroyAudio(const std::string& name);
            void DestroyAllAudio();

            [[nodiscard]] std::expected<AudioSourceId, bool> CreateSource(
                const std::string& fileName,
                const SourceProperties& properties);
            void DestroySource(const AudioSourceId& sourceId);
            bool PlaySource(const AudioSourceId& sourceId);
            bool IsSourceStopped(const AudioSourceId& sourceId);
            bool StopSource(const AudioSourceId& sourceId);
            bool UpdateSourceProperties(const AudioSourceId& sourceId, const glm::vec3& position);

            void FulfillFinishedGlobalSources();
            void UpdateListenerProperties(const AudioListener& listener) const;

        private:

            struct BufferProperties
            {
                BufferProperties(ALuint _bufferId, ALenum _bufferFormat)
                    : bufferId(_bufferId)
                    , bufferFormat(_bufferFormat)
                { }

                ALuint bufferId;
                ALenum bufferFormat;
                std::unordered_set<ALuint> sources;
            };

        private:

            Common::ILogger::Ptr m_logger;

            ALCdevice* m_pDevice{nullptr};
            ALCcontext* m_pContext{nullptr};

            // Buffer name (file name) -> Buffer properties
            std::unordered_map<std::string, BufferProperties> m_buffers;

            // SourceId -> Source properties
            std::unordered_map<ALuint, SourceProperties> m_sources;

            // SourceId -> Buffer name
            std::unordered_map<ALuint, std::string> m_sourceToBuffer;
    };
}

#endif //LIBACCELAENGINE_SRC_AUDIO_AUDIOMANAGER_H
