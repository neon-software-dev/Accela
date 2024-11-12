/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_SWRCONFIG_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_SWRCONFIG_H

extern "C"
{
    #include <libavutil/channel_layout.h>
    #include <libavutil/samplefmt.h>
}

namespace Accela::Engine
{
    /**
     * Configuration parameters for a swr context which resamples audio
     */
    struct SWRConfig
    {
        SWRConfig(AVChannelLayout _srcChannelLayout,
                  AVSampleFormat _srcSampleFormat,
                  int _srcSampleRate,
                  AVChannelLayout _destChannelLayout,
                  AVSampleFormat _destSampleFormat,
                  int _destSampleRate)
            : srcChannelLayout(_srcChannelLayout)
            , srcSampleFormat(_srcSampleFormat)
            , srcSampleRate(_srcSampleRate)
            , destChannelLayout(_destChannelLayout)
            , destSampleFormat(_destSampleFormat)
            , destSampleRate(_destSampleRate)
        { }

        [[nodiscard]] bool operator==(const SWRConfig& other) const
        {
            return
                // Src
                av_channel_layout_compare(&srcChannelLayout, &other.srcChannelLayout) == 0 &&
                srcSampleFormat == other.srcSampleFormat &&
                srcSampleRate == other.srcSampleRate &&
                // Dest
                av_channel_layout_compare(&destChannelLayout, &other.destChannelLayout) == 0 &&
                destSampleFormat == other.destSampleFormat &&
                destSampleRate == other.destSampleRate;
        }

        AVChannelLayout srcChannelLayout;
        AVSampleFormat srcSampleFormat;
        int srcSampleRate;

        AVChannelLayout destChannelLayout;
        AVSampleFormat destSampleFormat;
        int destSampleRate;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_SWRCONFIG_H
