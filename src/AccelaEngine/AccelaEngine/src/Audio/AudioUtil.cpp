/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioUtil.h"

#include <bit>
#include <array>
#include <algorithm>

namespace Accela::Engine
{

/**
* Takes an audio sample value in the range [-1,1] and returns the value
* mapped to an [0..UINT8_MAX] uint8_t
*
* @tparam T The sample data type
* @param sample The sample to be converted
*
* @return A [0..255] uint8_t representing the sample value
*/
template <class T>
static uint8_t SampleToUint8(const T& sample)
{
    T outSample = std::clamp(sample, -1.0, 1.0);
    outSample = (outSample + 1.0) / 2.0;
    return static_cast<uint8_t>(outSample * UINT8_MAX);
}

/**
 * Takes an audio sample value in the range [-1,1] and returns the value
 * mapped to an [-INT16_MAX, INT16_MAX] int16_t
 *
 * @tparam T The sample data type
 * @param sample The sample to be converted
 *
 * @return A [-INT16_MAX, INT16_MAX] int16_t representing the sample value
 */
template <class T>
static int16_t SampleToInt16(const T& sample)
{
    T outSample = std::clamp(sample, -1.0, 1.0);
    return static_cast<int16_t>(outSample * INT16_MAX);
}

/**
 * Converts an int16_t to an array of two bytes, ordered as appropriate for the machine's endianness
 */
std::array<std::byte, 2> Int16ToBytes(const int16_t& i)
{
    std::array<std::byte, 2> bytes{std::byte(0), std::byte(0)};

    if constexpr (std::endian::native == std::endian::little)
    {
        bytes[1] = std::byte((i >> 8) & 0xFF);
        bytes[0] = std::byte(i & 0xFF);
    }
    else
    {
        bytes[0] = std::byte((i >> 8) & 0xFF);
        bytes[1] = std::byte(i & 0xFF);
    }

    return bytes;
}

void AudioUtil::AppendSample(std::vector<std::byte>& byteBuffer, const int& bitDepth, const double& sample)
{
    if (bitDepth == 8)
    {
        byteBuffer.push_back(std::byte(SampleToUint8(sample)));
    }
    else
    {
        // All bit depths >= 16 get converted to 16 bit as that's the max OpenAL supports
        const std::array<std::byte, 2> bytes = Int16ToBytes(SampleToInt16(sample));
        byteBuffer.push_back(bytes[0]);
        byteBuffer.push_back(bytes[1]);
    }
}

std::vector<std::byte> AudioUtil::AudioFileToByteBuffer(const AudioFile<double>& audioFile)
{
    const int numChannels = audioFile.getNumChannels();
    const int numSamples = audioFile.getNumSamplesPerChannel();
    const int bitDepth = audioFile.getBitDepth();

    std::vector<std::byte> byteBuffer;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            // Packing format: [sample1.chan1, sample1.chan2, sample2.chan1, sample2.chan2, etc]
            AppendSample(byteBuffer, bitDepth, audioFile.samples[channel][sample]);
        }
    }

    return byteBuffer;
}

std::expected<Common::AudioData::Ptr, int>
AudioUtil::CombineAudioDatas(const std::vector<Common::AudioData::Ptr>& audioDatas)
{
    if (audioDatas.empty())
    {
        return std::unexpected(1);
    }

    std::vector<std::byte> combinedAudioData;
    std::optional<uint32_t> audioSampleRate;
    std::optional<Common::AudioDataFormat> audioFormat;

    for (const auto& audioData : audioDatas)
    {
        if (!audioSampleRate) { audioSampleRate = audioData->sampleRate; }
        if (!audioFormat) { audioFormat = audioData->format; }

        if (*audioSampleRate != audioData->sampleRate) { return std::unexpected(2); }
        if (*audioFormat != audioData->format) { return std::unexpected(3); }

        std::ranges::copy(audioData->data, std::back_inserter(combinedAudioData));
    }

    return std::make_shared<Common::AudioData>(*audioFormat, *audioSampleRate, std::move(combinedAudioData));
}

}

