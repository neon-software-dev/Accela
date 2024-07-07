/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Common/ImageData.h>

#include <cassert>

namespace Accela::Common
{

ImageData::ImageData(std::vector<std::byte> pixelBytes,
                     uint32_t numLayers,
                     std::size_t pixelWidth,
                     std::size_t pixelHeight,
                     ImageData::PixelFormat pixelFormat)
    : m_pixelBytes(std::move(pixelBytes))
    , m_numLayers(numLayers)
    , m_pixelWidth(pixelWidth)
    , m_pixelHeight(pixelHeight)
    , m_pixelFormat(pixelFormat)
{
    assert(SanityCheckValues());
}

Common::ImageData::Ptr ImageData::Clone() const
{
    return std::make_shared<ImageData>(
        m_pixelBytes,
        m_numLayers,
        m_pixelWidth,
        m_pixelHeight,
        m_pixelFormat
    );
}

uint8_t ImageData::GetBytesPerPixel() const
{
    switch (m_pixelFormat)
    {
        case PixelFormat::RGB24: return 3;
        case PixelFormat::R32G32: return 8;
        case PixelFormat::RGBA32: return 4;
    }

    assert(false);
    return 0;
}

std::vector<std::byte> ImageData::GetPixelBytes(const uint32_t& layerIndex, const uintmax_t& pixelIndex) const
{
    assert(layerIndex < GetNumLayers());
    assert(pixelIndex < GetLayerNumPixels());

    const auto valueStartByte = (layerIndex * GetLayerByteSize()) + pixelIndex * GetBytesPerPixel();
    const auto valueEndByte = valueStartByte + GetBytesPerPixel();

    return {
        m_pixelBytes.cbegin() + static_cast<long>(valueStartByte),
        m_pixelBytes.cbegin() + static_cast<long>(valueEndByte)
    };
}

bool ImageData::SanityCheckValues() const
{
    return m_pixelBytes.size() == m_pixelWidth * m_pixelHeight * m_numLayers * GetBytesPerPixel();
}

}
