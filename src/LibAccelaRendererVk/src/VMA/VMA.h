/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VMA_VMA
#define LIBACCELARENDERERVK_SRC_VMA_VMA

#include "IVMA.h"

#include "ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <optional>

namespace Accela::Render
{
    class VMA : public IVMA
    {
        public:

            explicit VMA(const VmaAllocator& vma);

            static std::optional<IVMAPtr> CreateInstance(const Common::ILogger::Ptr& logger,
                                                         const VmaAllocatorCreateInfo& createInfo);

            //
            // IVMA
            //
            void DestroyInstance() override;

            VkResult CreateBuffer(const VkBufferCreateInfo* pBufferCreateInfo,
                                  const VmaAllocationCreateInfo* pAllocationCreateInfo,
                                  VkBuffer* pBuffer,
                                  VmaAllocation* pAllocation,
                                  VmaAllocationInfo* pAllocationInfo) const override;
            void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation) const override;

            VkResult CreateImage(const VkImageCreateInfo* pImageCreateInfo,
                                 const VmaAllocationCreateInfo* pAllocationCreateInfo,
                                 VkImage* pImage,
                                 VmaAllocation* pAllocation,
                                 VmaAllocationInfo* pAllocationInfo) const override;
            void DestroyImage(VkImage image, VmaAllocation allocation) const override;

            VkResult MapMemory(VmaAllocation allocation, void** ppData) const override;
            void UnmapMemory(VmaAllocation allocation) const override;

        private:

            VmaAllocator m_vma;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VMA_VMA
