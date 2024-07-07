/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_IMAGE_H
#define LIBACCELARENDERERVK_SRC_IMAGE_IMAGE_H

#include "../VMA/vma_access.h"

#include <Accela/Render/Util/Rect.h>

#include <vulkan/vulkan.h>

#include <string>

namespace Accela::Render
{
    struct Image
    {
        std::string tag{"Unknown"};
        VkImageType vkImageType{VK_IMAGE_TYPE_2D};
        VkFormat vkFormat{VK_FORMAT_R8G8B8A8_SRGB};
        VkImageTiling vkImageTiling{VK_IMAGE_TILING_OPTIMAL};
        VkImageUsageFlags vkImageUsageFlags{VK_IMAGE_USAGE_SAMPLED_BIT};
        USize size{0,0};
        uint32_t numLayers{1};

        VkImageLayout vkInitialLayout{VK_IMAGE_LAYOUT_UNDEFINED};
        uint32_t numMipLevels{1};
        bool cubeCompatible{false};
        VmaMemoryUsage vmaMemoryUsage{VMA_MEMORY_USAGE_AUTO};
        VmaAllocationCreateFlags vmaAllocationCreateFlags{};
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_IMAGE_H
