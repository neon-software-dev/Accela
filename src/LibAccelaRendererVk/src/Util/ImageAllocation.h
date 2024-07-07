/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_IMAGEALLOCATION_H
#define LIBACCELARENDERERVK_SRC_UTIL_IMAGEALLOCATION_H

#include "../VMA/vma_access.h"

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    struct ImageAllocation
    {
        VkImage vkImage{VK_NULL_HANDLE};
        VmaAllocationCreateInfo vmaAllocationCreateInfo{};
        VmaAllocation vmaAllocation{nullptr};
        VmaAllocationInfo vmaAllocationInfo{};
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_IMAGEALLOCATION_H
