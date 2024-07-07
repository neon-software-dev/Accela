/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_RENDERTEXTURE_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_RENDERTEXTURE_H

#include <Accela/Render/Texture/Texture.h>
#include <Accela/Render/Texture/TextureView.h>
#include <Accela/Render/Texture/TextureSampler.h>

#include "../VMA/vma_access.h"

#include <vector>

namespace Accela::Render
{
    struct RenderTexture
    {
        RenderTexture(TextureId _id,
                      VkImageUsageFlags _vkUsageFlags,
                      VmaMemoryUsage _vmaMemoryUsage,
                      VmaAllocationCreateFlags _vmaAllocationCreateFlags,
                      std::optional<VkFormat> _format,
                      VkImageTiling _tiling,
                      USize _pixelSize,
                      uint32_t _numLayers,
                      std::optional<uint32_t> _numMipLevels,
                      std::string _tag)
            : id(_id)
            , vkUsageFlags(_vkUsageFlags)
            , vmaMemoryUsage(_vmaMemoryUsage)
            , vmaAllocationCreateFlags(_vmaAllocationCreateFlags)
            , format(_format)
            , tiling(_tiling)
            , pixelSize(_pixelSize)
            , numLayers(_numLayers)
            , numMipLevels(_numMipLevels)
            , tag(std::move(_tag))
        {}

        TextureId id{INVALID_ID};
        VkImageUsageFlags vkUsageFlags{};
        VmaMemoryUsage vmaMemoryUsage{};
        VmaAllocationCreateFlags vmaAllocationCreateFlags{};
        std::optional<VkFormat> format; // Left unset for depth textures, as the renderer internally determines depth format
        VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
        USize pixelSize;
        uint32_t numLayers{1};
        std::optional<uint32_t> numMipLevels;
        std::string tag;
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_RENDERTEXTURE_H
