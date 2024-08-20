/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_GPUDATABUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_GPUDATABUFFER_H

#include "DataBuffer.h"

namespace Accela::Render
{
    class GPUDataBuffer : public DataBuffer
    {
        public:

            static std::expected<DataBufferPtr, bool> Create(
                const IBuffersPtr& buffers,
                const PostExecutionOpsPtr& postExecutionOps,
                VkBufferUsageFlagBits bufferUsage,
                VkPipelineStageFlagBits firstUsageStage,
                VkPipelineStageFlagBits lastUsageStage,
                const std::size_t& initialCapacity,
                const std::string& tag
            );

        public:

            GPUDataBuffer(IBuffersPtr buffers,
                          PostExecutionOpsPtr postExecutionOps,
                          BufferPtr buffer,
                          VkPipelineStageFlagBits vkFirstUsageStage,
                          VkPipelineStageFlagBits vkLastUsageStage,
                          const std::size_t& initialByteSize);

            bool PushBack(const ExecutionContext& context, const BufferAppend& bufferAppend) override;
            bool Update(const ExecutionContext& context, const std::vector<BufferUpdate>& bufferUpdates) override;
            bool Delete(const ExecutionContext& context, const std::vector<BufferDelete>& bufferDeletes) override;
            bool Resize(const ExecutionContext& context, const std::size_t& byteSize) override;
            bool Reserve(const ExecutionContext& context, const std::size_t& byteSize) override;

        private:

            bool ResizeBuffer(const ExecutionContext& context, const std::size_t& newByteSize);

        private:

            PostExecutionOpsPtr m_postExecutionOps;
            VkPipelineStageFlagBits m_vkFirstUsageStage;
            VkPipelineStageFlagBits m_vkLastUsageStage;
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_GPUDATABUFFER_H
