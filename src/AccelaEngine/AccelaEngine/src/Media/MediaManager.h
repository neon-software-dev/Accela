/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIAMANAGER_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIAMANAGER_H

#include "IMediaSource.h"
#include "MediaSession.h"

#include "../ForwardDeclares.h"

#include <Accela/Engine/ResourceIdentifier.h>
#include <Accela/Engine/Audio/AudioSourceProperties.h>
#include <Accela/Engine/Media/MediaCommon.h>

#include <Accela/Render/IRenderer.h>

#include <Accela/Common/IdSource.h>
#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <expected>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Accela::Engine
{
    class MediaManager
    {
        public:

            MediaManager(Common::ILogger::Ptr logger,
                         Common::IMetrics::Ptr metrics,
                         IWorldResourcesPtr worldResources,
                         AudioManagerPtr audioManager,
                         Render::IRenderer::Ptr renderer);

            [[nodiscard]] bool Startup();
            void Shutdown();

            [[nodiscard]] std::expected<MediaSessionId, bool> CreateURLMediaSession(const std::string& url,
                                                                                    const AudioSourceProperties& audioSourceProperties,
                                                                                    bool localAudioSource);

            [[nodiscard]] bool DoesMediaSessionExist(const MediaSessionId& mediaSessionId) const;
            [[nodiscard]] std::optional<Render::TextureId> GetMediaSessionTextureId(const MediaSessionId& mediaSessionId) const;
            [[nodiscard]] std::optional<AudioSourceId> GetMediaSessionAudioSourceId(const MediaSessionId& mediaSessionId) const;

            [[nodiscard]] std::future<bool> PlayMediaSession(const MediaSessionId& mediaSessionId, const std::optional<MediaPoint>& playPoint) const;
            [[nodiscard]] std::future<bool> PauseMediaSession(const MediaSessionId& mediaSessionId) const;
            [[nodiscard]] std::future<bool> StopMediaSession(const MediaSessionId& mediaSessionId) const;
            [[nodiscard]] std::future<bool> SeekMediaSessionByOffset(const MediaSessionId& mediaSessionId, const MediaDuration& offset) const;
            [[nodiscard]] std::future<bool> LoadStreams(const MediaSessionId& mediaSessionId, const std::unordered_set<unsigned int>& streamIndices) const;

            void DestroySession(const MediaSessionId& mediaSessionId);
            void DestroyAll();

        private:

            [[nodiscard]] std::expected<MediaSessionId, bool> CreateFFMPEGURLSession(const std::string& url,
                                                                                     const AudioSourceProperties& audioSourceProperties,
                                                                                     bool localAudioSource);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResourcesPtr m_worldResources;
            AudioManagerPtr m_audioManager;
            Render::IRenderer::Ptr m_renderer;

            Common::IdSource<MediaSessionId> m_ids;

            std::unordered_map<MediaSessionId, std::unique_ptr<MediaSession>> m_sessions;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIAMANAGER_H
