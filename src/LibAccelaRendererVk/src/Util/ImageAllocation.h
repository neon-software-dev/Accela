#ifndef LIBACCELARENDERERVK_SRC_UTIL_IMAGEALLOCATION_H
#define LIBACCELARENDERERVK_SRC_UTIL_IMAGEALLOCATION_H

#include "../VMA/vma_access.h"

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    struct ImageAllocation
    {
        ImageAllocation() = default;

        ImageAllocation(const VkImage& _vkImage, const VmaAllocation& _vmaAllocation)
            : vkImage(_vkImage)
            , vmaAllocation(_vmaAllocation)
        { }

        VkImage vkImage{VK_NULL_HANDLE};
        VmaAllocation vmaAllocation{nullptr};
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_IMAGEALLOCATION_H
