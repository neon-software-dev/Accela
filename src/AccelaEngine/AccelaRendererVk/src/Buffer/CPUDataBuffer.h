/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_CPUDATABUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_CPUDATABUFFER_H

#include "DataBuffer.h"

namespace Accela::Render
{
    // TODO! The CPU variants of buffer helpers aren't synchronized when data is mapped updated
    class CPUDataBuffer : public DataBuffer
    {
        public:

            static std::expected<DataBufferPtr, bool> Create(const IBuffersPtr& buffers,
                                                            VkBufferUsageFlags vkUsageFlags,
                                                            const std::size_t& initialCapacity,
                                                            const std::string& tag);

            CPUDataBuffer(IBuffersPtr buffers, BufferPtr buffer, const std::size_t& initialByteSize);

            bool PushBack(const ExecutionContext& context, const BufferAppend& bufferAppend) override;
            bool Update(const ExecutionContext& context, const std::vector<BufferUpdate>& bufferUpdates) override;
            bool Delete(const ExecutionContext& context, const std::vector<BufferDelete>& bufferDeletes) override;
            bool Resize(const ExecutionContext& context, const std::size_t& byteSize) override;
            bool Reserve(const ExecutionContext& context, const std::size_t& byteSize) override;

        private:

            bool ResizeBuffer(const std::size_t& newByteSize);
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_CPUDATABUFFER_H
