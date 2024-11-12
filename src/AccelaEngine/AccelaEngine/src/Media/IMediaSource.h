/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_VIDEO_IMEDIASOURCE_H
#define ACCELAENGINE_ACCELAENGINE_SRC_VIDEO_IMEDIASOURCE_H

#include "MediaCommon.h"

#include <memory>
#include <optional>
#include <unordered_set>

namespace Accela::Engine
{
    class IMediaSource
    {
        public:

            using Ptr = std::shared_ptr<IMediaSource>;

        public:

            virtual ~IMediaSource() = default;

            [[nodiscard]] virtual std::size_t GetVideoFrameQueueSize() const = 0;
            [[nodiscard]] virtual std::optional<VideoFrame> PeekFrontVideoFrame() const = 0;
            [[nodiscard]] virtual std::optional<VideoFrame> PopFrontVideoFrame() = 0;

            [[nodiscard]] virtual std::size_t GetAudioFrameQueueSize() const = 0;
            [[nodiscard]] virtual std::optional<AudioFrame> PeekFrontAudioFrame() const = 0;
            [[nodiscard]] virtual std::optional<AudioFrame> PopFrontAudioFrame() = 0;

            [[nodiscard]] virtual MediaDuration GetSourceDuration() const = 0;

            [[nodiscard]] virtual bool HasHitEnd() const = 0;
            [[nodiscard]] virtual bool LoadFromPoint(MediaPoint mediaPoint, const std::optional<MediaDuration>& loadOffset) = 0;
            [[nodiscard]] virtual bool LoadStreams(MediaPoint curPoint, const std::unordered_set<unsigned int>& streamIndices) = 0;
            virtual void SetAudioSyncDiff(const MediaDuration& audioSyncDiff) = 0;
            virtual void Stop() = 0;

            virtual void Destroy() = 0;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_VIDEO_IMEDIASOURCE_H
