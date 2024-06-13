/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H

#include "../Id.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/ImageData.h>

#include <optional>
#include <string>
#include <utility>

namespace Accela::Render
{
    enum class TextureUsage
    {
        ImageMaterial,
        ImageCubeMaterial,
        ColorAttachment,
        DepthAttachment,
        DepthCubeAttachment,
        InputAttachment_RGBA16_SFLOAT,
        InputAttachment_R32_UINT
    };

    /**
     * Defines texture data for the renderer to render
     */
    struct Texture
    {
        static Texture Empty(TextureId id, TextureUsage usage, const USize& pixelSize, const uint32_t& numLayers, const std::string& tag)
        {
            Texture texture{};
            texture.id = id;
            texture.usage = usage;
            texture.pixelSize = pixelSize;
            texture.numLayers = numLayers;
            texture.data = std::nullopt;
            texture.tag = tag;
            return texture;
        }

        static Texture FromImageData(TextureId id, TextureUsage usage, const uint32_t& numLayers, const Common::ImageData::Ptr& data, const std::string& tag)
        {
            Texture texture{};
            texture.id = id;
            texture.usage = usage;
            texture.pixelSize = USize(data->GetPixelWidth(), data->GetPixelHeight());
            texture.numLayers = numLayers;
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
        TextureUsage usage{TextureUsage::ImageMaterial};
        USize pixelSize;
        uint32_t numLayers{1};
        std::optional<unsigned int> numMipLevels;
        std::optional<Common::ImageData::Ptr> data;
        std::string tag;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H
