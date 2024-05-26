/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "GPUDataBuffer.h"
#include "IBuffers.h"

#include "../PostExecutionOp.h"

#include <cassert>

namespace Accela::Render
{

std::expected<DataBufferPtr, bool>
GPUDataBuffer::Create(const IBuffersPtr& buffers,
                      const PostExecutionOpsPtr& postExecutionOps,
                      VkBufferUsageFlagBits bufferUsage,
                      VkPipelineStageFlagBits firstUsageStage,
                      VkPipelineStageFlagBits lastUsageStage,
                      const size_t& initialCapacity,
                      const std::string& tag)
{
    const auto bufferCreate = buffers->CreateBuffer(
        bufferUsage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        initialCapacity,
        tag
    );
    if (!bufferCreate.has_value())
    {
        return std::unexpected(false);
    }

    return std::make_shared<GPUDataBuffer>(buffers, postExecutionOps, bufferCreate.value(), firstUsageStage, lastUsageStage, 0);
}

GPUDataBuffer::GPUDataBuffer(IBuffersPtr buffers,
                             PostExecutionOpsPtr postExecutionOps,
                             BufferPtr buffer,
                             VkPipelineStageFlagBits vkFirstUsageStage,
                             VkPipelineStageFlagBits vkLastUsageStage,
                             const std::size_t& initialByteSize)
    : DataBuffer(std::move(buffers), std::move(buffer), initialByteSize)
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_vkFirstUsageStage(vkFirstUsageStage)
    , m_vkLastUsageStage(vkLastUsageStage)
{

}

bool GPUDataBuffer::PushBack(const ExecutionContext& context, const BufferAppend& bufferAppend)
{
    assert(context.type == ExecutionContext::Type::GPU);
    if (context.type != ExecutionContext::Type::GPU) { return false; }

    //
    // Make sure we have enough capacity to append the data
    //
    if (!Reserve(context, m_dataByteSize + bufferAppend.dataByteSize))
    {
        return false;
    }

    //
    // Update the buffer to write the data into unused capacity
    //
    BufferUpdate bufferUpdate{};
    bufferUpdate.pData = bufferAppend.pData;
    bufferUpdate.dataByteSize = bufferAppend.dataByteSize;
    bufferUpdate.updateOffset = m_dataByteSize;

    if (!Update(context, {bufferUpdate}))
    {
        return false;
    }

    //
    // Update internal state
    //
    m_dataByteSize += bufferAppend.dataByteSize;

    return true;
}

bool GPUDataBuffer::Update(const ExecutionContext& context, const std::vector<BufferUpdate>& bufferUpdates)
{
    assert(context.type == ExecutionContext::Type::GPU);
    if (context.type != ExecutionContext::Type::GPU) { return false; }

    return m_buffers->StagingUpdateBuffer(
        m_buffer,
        bufferUpdates,
        m_vkFirstUsageStage,
        m_vkLastUsageStage,
        context.commandBuffer,
        context.vkFence
    );
}

bool GPUDataBuffer::Delete(const ExecutionContext& context, const std::vector<BufferDelete>& bufferDeletes)
{
    assert(context.type == ExecutionContext::Type::GPU);
    if (context.type != ExecutionContext::Type::GPU) { return false; }

    if (bufferDeletes.empty()) { return true; }

    //
    // Delete the data sections
    //
    std::size_t totalBytesToDelete{0};

    for (const auto& bufferDelete : bufferDeletes)
    {
        totalBytesToDelete += bufferDelete.deleteByteSize;
    }

    if (!m_buffers->StagingDeleteData(m_buffer, bufferDeletes, m_vkFirstUsageStage, m_vkLastUsageStage, context.commandBuffer))
    {
        return false;
    }

    //
    // Resize the buffer down to its new size
    //
    return Resize(context, m_dataByteSize - totalBytesToDelete);
}

bool GPUDataBuffer::Resize(const ExecutionContext& context, const std::size_t& byteSize)
{
    assert(context.type == ExecutionContext::Type::GPU);
    if (context.type != ExecutionContext::Type::GPU) { return false; }

    // Ensure we have enough capacity in the buffer for the new size
    if (!Reserve(context, byteSize))
    {
        return false;
    }

    // Update our size
    m_dataByteSize = byteSize;

    // If our size is <= a quarter of our capacity, cut our capacity in half
    if (m_dataByteSize <= m_buffer->GetByteSize() / 4)
    {
        ResizeBuffer(context, m_buffer->GetByteSize() / 2);
    }

    return true;
}

bool GPUDataBuffer::Reserve(const ExecutionContext& context, const std::size_t& byteSize)
{
    assert(context.type == ExecutionContext::Type::GPU);
    if (context.type != ExecutionContext::Type::GPU) { return false; }

    if (m_buffer->GetByteSize() >= byteSize)
    {
        return true;
    }

    return ResizeBuffer(context, byteSize * 2);
}

bool GPUDataBuffer::ResizeBuffer(const ExecutionContext& context, const std::size_t& newByteSize)
{
    //
    // Create the new buffer
    //
    const auto bufferCreate = m_buffers->CreateBuffer(
        m_buffer->GetUsageFlags() | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        newByteSize,
        m_buffer->GetTag()
    );
    if (!bufferCreate.has_value())
    {
        return false;
    }

    //
    // Copy data from the old buffer into the new buffer
    //
    if (m_dataByteSize > 0)
    {
        if (!m_buffers->CopyBufferData(
            m_buffer,
            0,
            m_dataByteSize,
            *bufferCreate,
            0,
            m_vkFirstUsageStage,
            m_vkLastUsageStage,
            context.commandBuffer
        ))
        {
            return false;
        }
    }

    //
    // Schedule the old buffer for deletion
    //
    m_postExecutionOps->Enqueue(context.vkFence, BufferDeleteOp(m_buffers, m_buffer->GetBufferId()));

    //
    // Update internal state
    //
    m_buffer = *bufferCreate;

    return true;
}

}
