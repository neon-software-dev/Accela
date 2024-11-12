/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIACOMMON_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIACOMMON_H

#include <Accela/Engine/Media/MediaCommon.h>

#include <Accela/Common/ImageData.h>
#include <Accela/Common/AudioData.h>
#include <Accela/Common/Build.h>

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <string>

namespace Accela::Engine
{
    enum class MediaStreamType
    {
        Video,
        Audio,
        Subtitle
    };

    SUPPRESS_IS_NOT_USED static std::string TagForMediaStreamType(MediaStreamType mediaStreamType)
    {
        switch (mediaStreamType)
        {
            case MediaStreamType::Video: return "Video";
            case MediaStreamType::Audio: return "Audio";
            case MediaStreamType::Subtitle: return "Subtitle";
        }

        return "Unknown";
    }

    struct VideoFrame
    {
        int64_t pts{0};                     // Raw presentation time from the video packet/stream
        double timeBase{0.0};               // Raw timestamp timebase from the video/packet stream
        MediaPoint presentPoint;            // Calculated presentation point from pts/timebase
        Common::ImageData::Ptr imageData;   // Image data contained within the frame
    };

    struct AudioFrame
    {
        int64_t pts{0};                     // Raw presentation time from the video packet/stream
        double timeBase{0.0};               // Raw timestamp timebase from the video/packet stream
        MediaPoint presentPoint;            // Calculated presentation point from pts/timebase
        Common::AudioData::Ptr audioData;   // Audio data contained within the frame
    };

    struct SubtitleFrame
    {
        std::string text;
    };

    struct SubtitleSource
    {
        SubtitleSource(std::string _url, unsigned int _subtitleIndex)
            : url(std::move(_url))
            , subtitleIndex(_subtitleIndex)
        { }

        auto operator<=>(const SubtitleSource&) const = default;

        std::string url;
        unsigned int subtitleIndex;
    };

    struct StreamInfo
    {
        StreamInfo(MediaStreamType _streamType, unsigned int _streamIndex)
            : streamType(_streamType)
            , streamIndex(_streamIndex)
        { }

        MediaStreamType streamType;
        unsigned int streamIndex;
        std::optional<unsigned int> subtitleIndex;

        unsigned int codecID{0};
        std::string codecName;
        std::unordered_map<std::string, std::string> metadata;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_MEDIACOMMON_H
