/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGMEDIASOURCE_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGMEDIASOURCE_H

#include "FFMPEGContainer.h"
#include "PacketReader.h"
#include "PacketDecoder.h"
#include "SubtitleDecoder.h"

#include "../IMediaSource.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <mutex>
#include <deque>

namespace Accela::Engine
{
    class FFMPEGMediaSource : public IMediaSource
    {
        public:

            FFMPEGMediaSource(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics, FFMPEGContainer::UPtr container);

            //
            // IMediaSource
            //
            [[nodiscard]] std::size_t GetVideoFrameQueueSize() const override;
            [[nodiscard]] std::optional<VideoFrame> PeekFrontVideoFrame() const override;
            [[nodiscard]] std::optional<VideoFrame> PopFrontVideoFrame() override;
            [[nodiscard]] std::size_t GetAudioFrameQueueSize() const override;
            [[nodiscard]] std::optional<AudioFrame> PeekFrontAudioFrame() const override;
            [[nodiscard]] std::optional<AudioFrame> PopFrontAudioFrame() override;
            [[nodiscard]] MediaDuration GetSourceDuration() const override;
            [[nodiscard]] bool HasHitEnd() const override;
            [[nodiscard]] bool LoadFromPoint(MediaPoint loadMediaPoint, const std::optional<MediaDuration>& loadOffset) override;
            [[nodiscard]] bool LoadStreams(MediaPoint curPoint, const std::unordered_set<unsigned int>& streamIndices) override;
            void SetAudioSyncDiff(const MediaDuration& audioSyncDiff) override;
            void Stop() override;

            void Destroy() override;

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            FFMPEGContainer::UPtr m_container;

            std::unique_ptr<PacketReader> m_packetReader;
            std::unique_ptr<PacketDecoder<VideoFrame>> m_videoDecoder;
            std::unique_ptr<PacketDecoder<AudioFrame>> m_audioDecoder;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGMEDIASOURCE_H
