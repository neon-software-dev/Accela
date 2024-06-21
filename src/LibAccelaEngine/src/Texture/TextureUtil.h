/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_TEXTURE_TEXTUREUTIL_H
#define LIBACCELAENGINE_SRC_TEXTURE_TEXTUREUTIL_H

#include <Accela/Render/Texture/Texture.h>

#include <Accela/Common/ImageData.h>

namespace Accela::Engine
{
    [[nodiscard]] static Render::TextureFormat PixelFormatToTextureFormat(const Common::ImageData::PixelFormat& format)
    {
        switch (format)
        {
            // TODO! R8G8B8_SRGB isn't supported by most devices, reject images with RGB24 format for rendering
            case Common::ImageData::PixelFormat::RGB24: return Render::TextureFormat::R8G8B8_SRGB;
            case Common::ImageData::PixelFormat::RGBA32: return Render::TextureFormat::R8G8B8A8_SRGB;
        }

        assert(false);
        return Render::TextureFormat::R8G8B8A8_SRGB;
    }
}

#endif //LIBACCELAENGINE_SRC_TEXTURE_TEXTUREUTIL_H
