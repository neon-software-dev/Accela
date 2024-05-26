/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioUtil.h"

#include <bit>
#include <array>

namespace Accela::Engine
{

template <class T>
uint8_t SampleToUint8(const T& sample)
{
    T outSample = std::clamp(sample, -1., 1.);
    outSample = (outSample + 1.) / 2.;
    return static_cast<uint8_t>(outSample * 255.);
}

template <class T>
int16_t SampleToInt16(const T& sample)
{
    T outSample = std::clamp(sample, -1., 1.);
    return static_cast<int16_t>(outSample * 32767.);
}

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

void AppendSample(std::vector<std::byte>& byteBuffer,
                  const int& bitDepth,
                  const double& sample)
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
    int numChannels = audioFile.getNumChannels();
    int numSamples = audioFile.getNumSamplesPerChannel();
    int bitDepth = audioFile.getBitDepth();

    std::vector<std::byte> byteBuffer;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            AppendSample(byteBuffer, bitDepth, audioFile.samples[channel][sample]);
        }
    }

    return byteBuffer;
}

}

