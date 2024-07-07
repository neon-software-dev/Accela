/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_IMAGEDATA_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_IMAGEDATA_H

#include <vector>
#include <memory>
#include <cstdint>
#include <cstddef>

namespace Accela::Common
{
    /**
     * Contains the data associated with a 2D image: pixels, a pixel format, and a width/height.
     */
    class ImageData
    {
        public:

            using Ptr = std::shared_ptr<ImageData>;

            enum class PixelFormat
            {
                RGB24,          // 3 bytes per pixel, for R, G, and B values
                R32G32,         // 8 bytes per pixel, for R and G values
                RGBA32          // 4 bytes per pixel, for R, G, B, and A values
            };

        public:

            /**
             * @param pixelBytes        The image's raw byte data
             * @param numLayers         Number of width x height layers in the data
             * @param pixelWidth        The pixel width of the image
             * @param pixelHeight       The pixel height of the image
             * @param pixelFormat       The pixel format the image data uses
             */
            ImageData(
                std::vector<std::byte> pixelBytes,
                uint32_t numLayers,
                std::size_t pixelWidth,
                std::size_t pixelHeight,
                PixelFormat pixelFormat);

            [[nodiscard]] Common::ImageData::Ptr Clone() const;

            /**
             * @return The raw bytes that make up the image
             */
            [[nodiscard]] const std::vector<std::byte>& GetPixelBytes() const noexcept { return m_pixelBytes; }

            /**
            * @return The number of layers in the image
            */
            [[nodiscard]] uint32_t GetNumLayers() const noexcept { return m_numLayers; }

            /**
             * @return The width, in pixels, of the image
             */
            [[nodiscard]] std::size_t GetPixelWidth() const noexcept { return m_pixelWidth; }

            /**
             * @return The height, in pixels, of the image
             */
            [[nodiscard]] std::size_t GetPixelHeight() const noexcept { return m_pixelHeight; }

            /**
             * @return The format defining the elements of each pixel
             */
            [[nodiscard]] PixelFormat GetPixelFormat() const noexcept { return m_pixelFormat; }

            /**
             * @return The total number of pixels in one layer of the image
             */
            [[nodiscard]] std::size_t GetLayerNumPixels() const noexcept { return m_pixelWidth * m_pixelHeight; }

            /**
             * @return The total byte size of one layer of the image
             */
            [[nodiscard]] uint64_t GetLayerByteSize() const noexcept { return GetLayerNumPixels() * GetBytesPerPixel(); }

            /**
             * @return The total byte size of the image
             */
            [[nodiscard]] uint64_t GetTotalByteSize() const noexcept { return m_pixelBytes.size(); }

            /**
             * @return The number of bytes which make up one pixel
             */
            [[nodiscard]] uint8_t GetBytesPerPixel() const;

            /**
             * Return the bytes associated with a given pixel index. Will return a vector with GetBytesPerPixel()
             * number of elements.
             *
             * @param layerIndex The layer index to retrieve the pixel bytes from [0 .. GetNumLayers() - 1]
             * @param pixelIndex The pixel index in question [0 .. GetLayerNumPixels() - 1]
             *
             * @return The bytes associated with the given pixel index
             */
            [[nodiscard]] std::vector<std::byte> GetPixelBytes(const uint32_t& layerIndex, const uintmax_t& pixelIndex) const;

        private:

            [[nodiscard]] bool SanityCheckValues() const;

        private:

            std::vector<std::byte> m_pixelBytes;        // Sequence of bytes representing individual RGB/RGBA/etc components
            uint32_t m_numLayers;                       // Number of width x height layers in the data
            std::size_t m_pixelWidth;                   // Width of the image, in pixels
            std::size_t m_pixelHeight;                  // Height of the image, in pixels
            PixelFormat m_pixelFormat;                  // Pixel format of the image.
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_IMAGEDATA_H
