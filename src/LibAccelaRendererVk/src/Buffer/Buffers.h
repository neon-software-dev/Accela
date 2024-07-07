/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_BUFFERS_H
#define LIBACCELARENDERERVK_SRC_BUFFER_BUFFERS_H

#include "IBuffers.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/IdSource.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <unordered_map>

namespace Accela::Render
{
    class Buffers : public IBuffers
    {
        public:

            Buffers(Common::ILogger::Ptr logger,
                    Common::IMetrics::Ptr metrics,
                    VulkanObjsPtr vulkanObjs,
                    PostExecutionOpsPtr postExecutionOps);

            bool Initialize() override;
            void Destroy() override;

            std::expected<BufferPtr, BufferCreateError> CreateBuffer(
                VkBufferUsageFlags vkUsageFlags,
                VmaMemoryUsage vmaMemoryUsage,
                VmaAllocationCreateFlags vmaAllocationCreateFlags,
                const std::size_t& byteSize,
                const std::string& tag) override;

            [[nodiscard]] bool DestroyBuffer(BufferId bufferId) override;

            //
            // Mapped (CPU-to-GPU) buffer operations
            //
            [[nodiscard]] std::vector<std::byte> MappedReadBuffer(const BufferPtr& buffer, const BufferRead& read) const override;

            [[nodiscard]] bool MappedUpdateBuffer(const BufferPtr& buffer, const std::vector<BufferUpdate>& updates) const override;

            [[nodiscard]] bool MappedCopyBufferData(const BufferPtr& srcBuffer,
                                                    const std::size_t& srcOffset,
                                                    const std::size_t& copyByteSize,
                                                    const BufferPtr& dstBuffer,
                                                    const std::size_t& dstOffset) const override;

            [[nodiscard]] bool MappedDeleteData(const BufferPtr& buffer, const std::vector<BufferDelete>& deletes) const override;

            //
            // GPU-only buffer operations
            //
            [[nodiscard]] bool StagingUpdateBuffer(const BufferPtr& buffer,
                                                   const std::vector<BufferUpdate>& updates,
                                                   VkPipelineStageFlagBits firstUsageStageFlag,
                                                   VkPipelineStageFlagBits lastUsageStageFlag,
                                                   const VulkanCommandBufferPtr& commandBuffer,
                                                   const VkFence& vkExecutionFence) override;

            [[nodiscard]] bool StagingDeleteData(const BufferPtr& buffer,
                                                 const std::vector<BufferDelete>& deletes,
                                                 VkPipelineStageFlagBits firstUsageStageFlag,
                                                 VkPipelineStageFlagBits lastUsageStageFlag,
                                                 const VulkanCommandBufferPtr& commandBuffer) const override;

            [[nodiscard]] bool CopyBufferData(const BufferPtr& srcBuffer,
                                              const std::size_t& srcOffset,
                                              const std::size_t& copyByteSize,
                                              const BufferPtr& dstBuffer,
                                              const std::size_t& dstOffset,
                                              const VkPipelineStageFlagBits& firstUsageStageFlag,
                                              const VkPipelineStageFlagBits& lastUsageStageFlag,
                                              const VulkanCommandBufferPtr& commandBuffer) const override;

        private:

            struct DeleteOffsetSort
            {
                bool operator()(const BufferDelete& lhs, const BufferDelete& rhs) const
                {
                    return lhs.deleteOffset < rhs.deleteOffset;
                }
            };

            struct BufferSection
            {
                std::size_t offset;
                std::size_t byteSize;
            };

        private:

            void SyncMetrics();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            VulkanObjsPtr m_vulkanObjs;
            PostExecutionOpsPtr m_postExecutionOps;

            IdSource<BufferId> m_bufferIds;
            std::unordered_map<BufferId, BufferPtr> m_buffers;
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_BUFFERS_H
