/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_LOADEDTEXTURE_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_LOADEDTEXTURE_H

#include "../InternalId.h"

#include "../Util/ImageAllocation.h"

#include <Accela/Render/Util/Rect.h>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>

namespace Accela::Render
{
    struct LoadedTexture
    {
        TextureId textureId{INVALID_ID};
        USize pixelSize{0,0};
        uint32_t mipLevels{1};
        uint32_t numLayers{1};

        VkFormat vkFormat{};
        ImageAllocation allocation;
        std::unordered_map<std::string, VkImageView> vkImageViews;
        VkSampler vkSampler{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_LOADEDTEXTURE_H
