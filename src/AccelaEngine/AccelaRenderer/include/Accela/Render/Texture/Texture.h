/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H

#include "../Id.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/ImageData.h>

#include <optional>
#include <string>
#include <utility>

namespace Accela::Render
{
    enum class Format
    {
        RGBA32
    };

    /**
     * Defines texture data for the renderer to render
     */
    struct ACCELA_PUBLIC Texture
    {
        static std::optional<Texture> FromImageData(TextureId id,
                                                    const uint32_t& numLayers,
                                                    bool cubicTexture,
                                                    const Common::ImageData::Ptr& data,
                                                    const std::string& tag)
        {
            Format imageFormat{Format::RGBA32};

            switch (data->GetPixelFormat())
            {
                case Common::ImageData::PixelFormat::RGBA32:
                    imageFormat = Format::RGBA32;
                break;
                default: return std::nullopt;
            }

            Texture texture{};
            texture.id = id;
            texture.format = imageFormat;
            texture.pixelSize = USize(data->GetPixelWidth(), data->GetPixelHeight());
            texture.numLayers = numLayers;
            texture.cubicTexture = cubicTexture;
            texture.data = data;
            texture.tag = tag;
            return texture;
        }

        /**
         * Automatically sets numMipLevels to "full" mip levels - the number of times the size
         * of the texture can be cut in half
         */
        void SetFullMipLevels()
        {
            numMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(pixelSize.w, pixelSize.h)))) + 1;
        }

        TextureId id{INVALID_ID};
        Format format;
        USize pixelSize;
        uint32_t numLayers{1};
        bool cubicTexture{false};
        std::optional<unsigned int> numMipLevels;
        Common::ImageData::Ptr data;
        std::string tag;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H
