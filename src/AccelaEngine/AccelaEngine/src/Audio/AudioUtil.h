/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_AUDIO_AUDIOUTIL_H
#define LIBACCELAENGINE_SRC_AUDIO_AUDIOUTIL_H

// Must be included before AudioFile.h as there's a bug in AudioFile lib
// where it'll fail to compile on newer C++ versions otherwise
#include <cstdint>

#include <AudioFile.h>

#include <Accela/Common/AudioData.h>

#include <vector>
#include <cstddef>
#include <array>
#include <cstdint>
#include <numeric>
#include <expected>

namespace Accela::Engine
{
    struct AudioUtil
    {
        /**
         * Appends a sample value (range of [-1,1]) to a byte buffer. Converts the sample to bytes as determined by
         * bitDepth parameter. A bitDepth of 8 results in a single byte sample value being appended, while any other
         * bitDepth results in a 16 bit sample value being appended.
         */
        static void AppendSample(std::vector<std::byte>& byteBuffer, const int& bitDepth, const double& sample);

        /**
         * Converts an AudioFile to a vector of bytes which represent the audio file
         */
        static std::vector<std::byte> AudioFileToByteBuffer(const AudioFile<double>& audioFile);

        static std::expected<Common::AudioData::Ptr, int> CombineAudioDatas(const std::vector<Common::AudioData::Ptr>& audioDatas);
    };
}

#endif //LIBACCELAENGINE_SRC_AUDIO_AUDIOUTIL_H
