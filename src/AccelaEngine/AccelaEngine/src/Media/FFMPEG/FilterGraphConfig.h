/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FILTERGRAPHCONFIG_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FILTERGRAPHCONFIG_H

#include "../MediaCommon.h"

extern "C"
{
    #include <libavutil/pixfmt.h>
    #include <libavutil/rational.h>
}

#include <optional>

namespace Accela::Engine
{
    /**
     * Configuration parameters for a filter graph which transforms frames
     */
    struct FilterGraphConfig
    {
        FilterGraphConfig(int _srcWidth,
                          int _srcHeight,
                          AVPixelFormat _srcPixelFormat,
                          AVRational _srcTimeBase,
                          AVRational _srcAspectRatio,
                          AVPixelFormat _destPixelFormat,
                          std::optional<SubtitleSource> _subtitleSource)
            : srcWidth(_srcWidth)
            , srcHeight(_srcHeight)
            , srcPixelFormat(_srcPixelFormat)
            , srcTimeBase(_srcTimeBase)
            , srcAspectRatio(_srcAspectRatio)
            , destPixelFormat(_destPixelFormat)
            , subtitleSource(std::move(_subtitleSource))
        { }

        [[nodiscard]] bool operator==(const FilterGraphConfig& other) const
        {
            return
                // Buffer Filter
                srcWidth == other.srcWidth &&
                srcHeight == other.srcHeight &&
                srcPixelFormat == other.srcPixelFormat &&
                av_cmp_q(srcTimeBase, other.srcTimeBase) == 0 &&
                av_cmp_q(srcAspectRatio, other.srcAspectRatio) == 0 &&
                destPixelFormat == other.destPixelFormat &&
                // Graph Filters
                subtitleSource == other.subtitleSource;
        }

        int srcWidth;
        int srcHeight;
        AVPixelFormat srcPixelFormat;
        AVRational srcTimeBase;
        AVRational srcAspectRatio;
        AVPixelFormat destPixelFormat;

        std::optional<SubtitleSource> subtitleSource;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FILTERGRAPHCONFIG_H
