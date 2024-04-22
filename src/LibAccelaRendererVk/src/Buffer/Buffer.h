/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_BUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_BUFFER_H

#include "../InternalId.h"

#include "../VMA/vma_access.h"

#include <vulkan/vulkan.h>

#include <string>

namespace Accela::Render
{
    struct BufferAllocation
    {
        VkBufferUsageFlags vkBufferUsageFlags{0};
        VmaMemoryUsage vmaMemoryUsage{VMA_MEMORY_USAGE_UNKNOWN};

        VkBuffer vkBuffer{VK_NULL_HANDLE};
        VmaAllocation vmaAllocation{nullptr};
    };

    struct BufferAppend
    {
        void const* pData{nullptr};
        std::size_t dataByteSize{0};
    };

    struct BufferUpdate
    {
        void const* pData{nullptr};
        std::size_t dataByteSize{0};
        std::size_t updateOffset{0};
    };

    struct BufferDelete
    {
        std::size_t deleteOffset{0};
        std::size_t deleteByteSize{0};
    };

    template <typename T>
    struct ItemUpdate
    {
        ItemUpdate() = default;

        ItemUpdate(const T& _item, const std::size_t& _position)
            : item(_item)
            , position(_position)
        { }

        T item{};
        std::size_t position{0};
    };

    class Buffer
    {
        public:

            Buffer(BufferId bufferId,
                   VkBufferUsageFlags vkUsageFlags,
                   BufferAllocation allocation,
                   std::size_t byteSize,
                   std::string tag)
               : m_bufferId(bufferId)
               , m_vkUsageFlags(vkUsageFlags)
               , m_allocation(allocation)
               , m_byteSize(byteSize)
               , m_tag(std::move(tag))
            { }

            [[nodiscard]] BufferId GetBufferId() const noexcept { return m_bufferId; }
            [[nodiscard]] VkBufferUsageFlags GetUsageFlags() const noexcept { return m_vkUsageFlags; }
            [[nodiscard]] const BufferAllocation& GetAllocation() const noexcept{ return m_allocation; }
            [[nodiscard]] VkBuffer GetVkBuffer() const noexcept{ return m_allocation.vkBuffer; }
            [[nodiscard]] VmaAllocation GetVmaAllocation() const noexcept{ return m_allocation.vmaAllocation; }
            [[nodiscard]] std::size_t GetByteSize() const noexcept{ return m_byteSize; }
            [[nodiscard]] std::string GetTag() const noexcept { return m_tag; }

        private:

            BufferId m_bufferId;
            VkBufferUsageFlags m_vkUsageFlags;
            BufferAllocation m_allocation;
            std::size_t m_byteSize;
            std::string m_tag;
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_BUFFER_H
