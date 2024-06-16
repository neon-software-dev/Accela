/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_IBUFFERS_H
#define LIBACCELARENDERERVK_SRC_BUFFER_IBUFFERS_H

#include "Buffer.h"

#include "../ForwardDeclares.h"

#include "../VMA/vma_access.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <string>
#include <cstdint>
#include <vector>

namespace Accela::Render
{
    class IBuffers
    {
        public:

            enum class BufferCreateError
            {
                ZeroSizeBuffer,
                AllocationFailed
            };

        public:

            virtual ~IBuffers() = default;

            virtual bool Initialize() = 0;
            virtual void Destroy() = 0;

            /**
             * Create a new buffer
             *
             * @param vkUsageFlags Vulkan buffer usage flags
             * @param vmaMemoryUsage Vma memory usage flags
             * @param byteSize Byte size of the buffer
             * @param tag Debug tag to associate with the buffer
             *
             * @return The created buffer
             */
            virtual std::expected<BufferPtr, BufferCreateError> CreateBuffer(
                VkBufferUsageFlags vkUsageFlags,
                VmaMemoryUsage vmaMemoryUsage,
                const std::size_t& byteSize,
                const std::string& tag) = 0;

            /**
             * Destroy the specified buffer
             */
            virtual bool DestroyBuffer(BufferId bufferId) = 0;

            /**
             * Updates a buffer by mapping it into memory and copying data into it.
             * The buffer must be a CPU-mappable buffer.
             *
             * @param buffer The buffer to be updated
             * @param updates The details of the updates
             *
             * @return Whether the buffer was updated successfully
             */
            [[nodiscard]] virtual bool MappedUpdateBuffer(const BufferPtr& buffer, const std::vector<BufferUpdate>& updates) const = 0;

            /**
             * Deletes data sections in a mappable buffer. The remaining data is tightly compacted
             * down to fill any holes.
             *
             * @param buffer The buffer to delete from
             * @param deletes The specific deletes to make
             *
             * @return Whether the data was deleted successfully
             */
            [[nodiscard]] virtual bool MappedDeleteData(const BufferPtr& buffer, const std::vector<BufferDelete>& deletes) const = 0;

            /**
             * Copies data between two mappable buffers.
             *
             * @param srcBuffer The source buffer to copy from
             * @param srcOffset The source byte offset to start copying from
             * @param copyByteSize The number of bytes to copy
             * @param dstBuffer The dest buffer to copy to
             * @param dstOffset The dest byte offset to start copying to
             *
             * @return Whether the copy was successful
             */
            [[nodiscard]] virtual bool MappedCopyBufferData(const BufferPtr& srcBuffer,
                                                            const std::size_t& srcOffset,
                                                            const std::size_t& copyByteSize,
                                                            const BufferPtr& dstBuffer,
                                                            const std::size_t& dstOffset) const = 0;

            /**
             * Updates a buffer by copying data into it from a staging buffer. Requires
             * command submission to execute the copy. Takes care of deleting its internal
             * staging buffer and creating a pipeline barrier to prevent reading from the
             * buffer again until the copy has finished.
             *
             * @param buffer The buffer to be updated
             * @param updates The details of the updates
             * @param firstUsageStageFlag The first pipeline stage that reads from the buffer. Used
             * when creating a pipeline barrier.
             * @param commandBuffer The command buffer to record commands into.
             * @param vkExecutionFence A fence that tracks execution of the command buffer's work
             *
             * @return Whether the buffer was updated successfully
             */
            [[nodiscard]] virtual bool StagingUpdateBuffer(const BufferPtr& buffer,
                                                           const std::vector<BufferUpdate>& updates,
                                                           VkPipelineStageFlagBits firstUsageStageFlag,
                                                           VkPipelineStageFlagBits lastUsageStageFlag,
                                                           const VulkanCommandBufferPtr& commandBuffer,
                                                           const VkFence& vkExecutionFence) = 0;

            /**
             * Deletes sections from a GPU buffer by issuing delete commands. After deletions
             * have been performed, tightly packs remaining data down to fill holes.
             *
             * @param buffer The buffer to delete from
             * @param deletes The deletes to perform
             * @param firstUsageStageFlag The first pipeline stage that uses the buffer (for synchronization)
             * @param commandBuffer The command buffer to record the commands into
             *
             * @return Whether the deletes could be enqueued successfully
             */
            [[nodiscard]]  virtual bool StagingDeleteData(const BufferPtr& buffer,
                                                          const std::vector<BufferDelete>& deletes,
                                                          VkPipelineStageFlagBits firstUsageStageFlag,
                                                          VkPipelineStageFlagBits lastUsageStageFlag,
                                                          const VulkanCommandBufferPtr& commandBuffer) const = 0;

            /**
             * Copies data from one buffer to another. The buffers can be of any type.
             *
             * @param srcBuffer The source buffer to copy from
             * @param srcOffset Byte offset in the source buffer to start the copy at
             * @param copyByteSize Byte size of data to be copied
             * @param dstBuffer The destination buffer to copy to
             * @param dstOffset Byte offset in the destination buffer to copy to
             * @param firstUsageStageFlag The first pipeline stage that reads from the buffer. Used
             * when creating a pipeline barrier.
             * @param commandBuffer The command buffer to record commands into.
             *
             * @return Whether the buffer was updated successfully
             */
            [[nodiscard]] virtual bool CopyBufferData(const BufferPtr& srcBuffer,
                                                      const std::size_t& srcOffset,
                                                      const std::size_t& copyByteSize,
                                                      const BufferPtr& dstBuffer,
                                                      const std::size_t& dstOffset,
                                                      const VkPipelineStageFlagBits& firstUsageStageFlag,
                                                      const VkPipelineStageFlagBits& lastUsageStageFlag,
                                                      const VulkanCommandBufferPtr& commandBuffer) const = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_IBUFFERS_H
