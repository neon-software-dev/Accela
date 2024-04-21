/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_TEXTUREDATA_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_TEXTUREDATA_H

#include "Accela/Common/ImageData.h"

#include <vector>

namespace Accela::Engine
{
    /**
     * Data associated with a loaded texture
     */
    struct TextureData
    {
        TextureData() = default;

        explicit TextureData(const Common::ImageData::Ptr& image)
        {
            textureImages.push_back(image);
        }

        explicit TextureData(std::vector<Common::ImageData::Ptr> images)
            : textureImages(std::move(images))
        { }

        // The ImageData(s) associated with the texture. Will be one element for
        // single image textures and six elements for cube textures.
        std::vector<Common::ImageData::Ptr> textureImages;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_TEXTUREDATA_H
