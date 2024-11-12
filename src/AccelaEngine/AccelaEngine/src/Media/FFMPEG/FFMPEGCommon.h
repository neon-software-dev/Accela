/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGCOMMON_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGCOMMON_H

#include <Accela/Common/Build.h>

extern "C"
{
    #include <libavutil/error.h>
};

#include <string>
#include <format>

namespace Accela::Engine
{
    /**
     * Returns an error string for an ffmpeg error code
     */
    SUPPRESS_IS_NOT_USED
    static std::string AVErrorStr(int errorCode)
    {
        char errStr[AV_ERROR_MAX_STRING_SIZE];
        if (av_strerror(errorCode, errStr, AV_ERROR_MAX_STRING_SIZE) != 0)
        {
            return std::format("ErrorCode({})", errorCode);
        }
        return errStr;
    }
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_FFMPEGCOMMON_H
