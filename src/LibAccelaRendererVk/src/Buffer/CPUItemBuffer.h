/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_CPUITEMBUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_CPUITEMBUFFER_H

#include "ItemBuffer.h"
#include "CPUDataBuffer.h"

namespace Accela::Render
{
    template <typename T>
    class CPUItemBuffer : public ItemBuffer<T>
    {
        public:

            static std::expected<std::shared_ptr<ItemBuffer<T>>, bool> Create(
                const IBuffersPtr& buffers,
                VkBufferUsageFlags vkUsageFlags,
                const std::size_t& initialCapacity,
                const std::string& tag)
            {
                const auto initialByteCapacity = initialCapacity * sizeof(T);

                auto dataBuffer = CPUDataBuffer::Create(buffers, vkUsageFlags, initialByteCapacity, tag);
                if (!dataBuffer)
                {
                    return std::unexpected(dataBuffer.error());
                }

                return std::make_shared<CPUItemBuffer>(dataBuffer.value(), 0);
            }

            explicit CPUItemBuffer(DataBufferPtr dataBuffer, const std::size_t& size)
                : ItemBuffer<T>(std::move(dataBuffer), size)
            { }
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_CPUITEMBUFFER_H
