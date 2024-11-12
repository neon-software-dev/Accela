/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIASESSION_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIASESSION_H

#include "IMediaSource.h"
#include "MediaCommon.h"
#include "Clock.h"

#include "../ForwardDeclares.h"

#include <Accela/Engine/Media/MediaCommon.h>
#include <Accela/Engine/Audio/AudioCommon.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/IRenderer.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Container/ConcurrentQueue.h>
#include <Accela/Common/Thread/Message.h>

#include <deque>
#include <queue>
#include <thread>
#include <chrono>
#include <optional>
#include <unordered_set>

namespace Accela::Engine
{
    class MediaSession
    {
        public:

            enum class MasterClockType
            {
                External,
                Video,
                Audio
            };

            enum class MediaState
            {
                Playing,
                Paused,
                Seeking,
                Stopped
            };

        public:

            MediaSession(Common::ILogger::Ptr logger,
                         Common::IMetrics::Ptr metrics,
                         Render::IRenderer::Ptr renderer,
                         AudioManagerPtr audioManager,
                         MediaSessionId mediaSessionId,
                         IMediaSource::Ptr mediaSource,
                         Common::ImageData::Ptr initialImage,
                         Render::TextureId textureId,
                         AudioSourceId audioSourceId,
                         MasterClockType masterClockType = MasterClockType::External);

            void Destroy();

            [[nodiscard]] MediaSessionId GetMediaSessionId() const;
            [[nodiscard]] Render::TextureId GetTextureId() const;
            [[nodiscard]] AudioSourceId GetAudioSourceId() const;

            [[nodiscard]] std::future<bool> Play(const std::optional<MediaPoint>& playPoint = std::nullopt);
            std::future<bool> Pause();
            std::future<bool> Stop();
            [[nodiscard]] std::future<bool> SeekByOffset(MediaDuration mediaDuration);
            [[nodiscard]] std::future<bool> SeekToPoint(MediaPoint mediaPoint);
            [[nodiscard]] std::future<bool> LoadStreams(const std::unordered_set<unsigned int>& streamIndices);

        private:

            void ThreadFunc();

            void Thread_ProcessCommands();
            [[nodiscard]] bool Thread_PlayCommand(const std::optional<MediaPoint>& playPoint);
            [[nodiscard]] bool Thread_PauseCommand();
            [[nodiscard]] bool Thread_StopCommand();
            [[nodiscard]] bool Thread_SeekByOffsetCommand(MediaDuration mediaDuration);
            [[nodiscard]] bool Thread_SeekToPointCommand(MediaPoint mediaPoint);
            [[nodiscard]] bool Thread_LoadStreamsCommand(const std::unordered_set<unsigned int>& streamIndices);

            [[nodiscard]] bool Thread_PresentVideoFrame(const std::chrono::steady_clock::time_point& now);
            void Thread_PresentAudioFrame(const std::chrono::steady_clock::time_point& now);

            void Thread_RecordAudioSyncDiff(const std::chrono::steady_clock::time_point& now);
            void Thread_ResetAudioSyncDiff();

            void Thread_UpdateVideoClock(const MediaPoint& syncPoint, const std::chrono::steady_clock::time_point& now);
            void Thread_UpdateAudioClock(const std::chrono::steady_clock::time_point& now);
            void Thread_UpdateExternalClock(const MediaPoint& syncPoint, const std::chrono::steady_clock::time_point& now);

            [[nodiscard]] const Clock& GetMasterClock() const;
            [[nodiscard]] std::optional<MediaPoint> GetMasterClockMediaPoint(const std::chrono::steady_clock::time_point& now) const;

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            Render::IRenderer::Ptr m_renderer;
            AudioManagerPtr m_audioManager;
            MediaSessionId m_mediaSessionId;
            IMediaSource::Ptr m_mediaSource;
            Common::ImageData::Ptr m_initialImage;
            Render::TextureId m_textureId;
            AudioSourceId m_audioSourceId;
            MasterClockType m_masterClockType{MasterClockType::External};

            bool m_doRunSession{true};
            std::thread m_sessionThread;
            Common::ConcurrentQueue<Common::Message::Ptr> m_commandQueue;

            MediaState m_mediaState{MediaState::Stopped};

            // The media state we were in when a seek was executed
            std::optional<MediaState> m_seekSourceState;

            Clock m_videoClock{};
            Clock m_audioClock{};
            Clock m_externalClock{};

            std::optional<MediaPoint> m_nextVideoPresentPoint;
            std::optional<MediaPoint> m_nextAudioPresentPoint;

            MediaDuration m_audioDiffCum{0.0};
            unsigned int m_audioDiffSamples = 0;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIASESSION_H
