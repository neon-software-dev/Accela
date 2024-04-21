/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_GPUITEMBUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_GPUITEMBUFFER_H

#include "ItemBuffer.h"
#include "GPUDataBuffer.h"

namespace Accela::Render
{
    template <typename T>
    class GPUItemBuffer : public ItemBuffer<T>
    {
        public:

            static std::expected<std::shared_ptr<ItemBuffer<T>>, bool> Create(
                const IBuffersPtr& buffers,
                const PostExecutionOpsPtr& postExecutionOps,
                VkBufferUsageFlagBits bufferUsage,
                VkPipelineStageFlagBits firstUsageStage,
                VkPipelineStageFlagBits lastUsageStage,
                const std::size_t& initialCapacity,
                const std::string& tag)
            {
                const auto initialByteCapacity = initialCapacity * sizeof(T);

                auto dataBuffer = GPUDataBuffer::Create(buffers, postExecutionOps, bufferUsage, firstUsageStage, lastUsageStage, initialByteCapacity, tag);
                if (!dataBuffer)
                {
                    return std::unexpected(dataBuffer.error());
                }

                return std::make_shared<GPUItemBuffer>(dataBuffer.value(), 0);
            }

            explicit GPUItemBuffer(DataBufferPtr dataBuffer, const std::size_t& size)
                : ItemBuffer<T>(std::move(dataBuffer), size)
            { }
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_GPUITEMBUFFER_H
