/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VMA_IVMA
#define LIBACCELARENDERERVK_SRC_VMA_IVMA

#include "vma_access.h"

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    /**
     * Interface for making calls to vma library
     */
    class IVMA
    {
        public:

            virtual ~IVMA() = default;

            virtual void DestroyInstance() = 0;

            virtual VkResult CreateBuffer(const VkBufferCreateInfo* pBufferCreateInfo,
                                          const VmaAllocationCreateInfo* pAllocationCreateInfo,
                                          VkBuffer* pBuffer,
                                          VmaAllocation* pAllocation,
                                          VmaAllocationInfo* pAllocationInfo) const = 0;
            virtual void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation) const = 0;

            virtual VkResult CreateImage(const VkImageCreateInfo* pImageCreateInfo,
                                         const VmaAllocationCreateInfo* pAllocationCreateInfo,
                                         VkImage* pImage,
                                         VmaAllocation* pAllocation,
                                         VmaAllocationInfo* pAllocationInfo) const = 0;
            virtual void DestroyImage(VkImage image, VmaAllocation allocation) const = 0;

            virtual VkResult MapMemory(VmaAllocation allocation, void** ppData) const = 0;
            virtual void UnmapMemory(VmaAllocation allocation) const = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VMA_IVMA
