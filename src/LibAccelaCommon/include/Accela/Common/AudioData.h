/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_AUDIODATA_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_AUDIODATA_H

#include <vector>
#include <cstdint>
#include <memory>

namespace Accela::Common
{
    struct AudioData
    {
        using Ptr = std::shared_ptr<AudioData>;

        enum class Format
        {
            Mono8,
            Mono16,
            Stereo8,
            Stereo16
        };

        AudioData(const Format& _format, const uint32_t& _sampleRate, std::vector<std::byte> _data)
            : format(_format)
            , sampleRate(_sampleRate)
            , data(std::move(_data))
        { }

        Format format;
        uint32_t sampleRate;
        std::vector<std::byte> data;
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_AUDIODATA_H
