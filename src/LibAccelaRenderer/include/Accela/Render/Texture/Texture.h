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
        Sampled,
        InputAttachment,
        ColorAttachment,
        DepthStencilAttachment,
        TransferSource,
        Storage
    };

    enum class TextureFormat
    {
        R32_UINT,           // 4 byte unsigned int
        R8G8B8_SRGB,        // RGB stored as single byte SRGB
        R8G8B8A8_SRGB,      // RGBA stored as single byte SRGB
        R32G32B32A32_SFLOAT // RGBA stored as 4 byte signed floats
    };

    /**
     * Defines texture data for the renderer to render
     */
    struct Texture
    {
        static Texture Empty(TextureId id,
                             const std::vector<TextureUsage>& usages,
                             TextureFormat format,
                             const USize& pixelSize,
                             const uint32_t& numLayers,
                             bool cubicTexture,
                             const std::string& tag)
        {
            Texture texture{};
            texture.id = id;
            texture.usages = usages;
            texture.format = format;
            texture.pixelSize = pixelSize;
            texture.numLayers = numLayers;
            texture.cubicTexture = cubicTexture;
            texture.data = std::nullopt;
            texture.tag = tag;
            return texture;
        }

        static Texture EmptyDepth(TextureId id,
                                  const std::vector<TextureUsage>& usages,
                                  const USize& pixelSize,
                                  const uint32_t& numLayers,
                                  bool cubicTexture,
                                  const std::string& tag)
        {
            Texture texture{};
            texture.id = id;
            texture.usages = usages;
            texture.pixelSize = pixelSize;
            texture.numLayers = numLayers;
            texture.cubicTexture = cubicTexture;
            texture.data = std::nullopt;
            texture.tag = tag;
            return texture;
        }

        static Texture FromImageData(TextureId id,
                                     const std::vector<TextureUsage>& usages,
                                     TextureFormat format,
                                     const uint32_t& numLayers,
                                     bool cubicTexture,
                                     const Common::ImageData::Ptr& data,
                                     const std::string& tag)
        {
            Texture texture{};
            texture.id = id;
            texture.usages = usages;
            texture.format = format;
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
        std::vector<TextureUsage> usages;
        std::optional<TextureFormat> format; // Left unset for depth textures, as the renderer internally determines depth format
        USize pixelSize;
        uint32_t numLayers{1};
        bool cubicTexture{false};
        std::optional<unsigned int> numMipLevels;
        std::optional<Common::ImageData::Ptr> data;
        std::string tag;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURE_H
