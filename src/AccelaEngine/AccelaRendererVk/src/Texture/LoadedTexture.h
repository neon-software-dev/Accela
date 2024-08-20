/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_LOADEDTEXTURE_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_LOADEDTEXTURE_H

#include "../InternalId.h"

#include "../Util/ImageAllocation.h"

#include <Accela/Render/Texture/TextureDefinition.h>
#include <Accela/Render/Util/Rect.h>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>

namespace Accela::Render
{
    struct LoadedTexture
    {
        TextureDefinition textureDefinition;
        ImageId imageId{INVALID_ID};
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_LOADEDTEXTURE_H
