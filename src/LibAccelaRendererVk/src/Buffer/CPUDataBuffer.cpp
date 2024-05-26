#include "CPUDataBuffer.h"

#include "IBuffers.h"

#include <cassert>

namespace Accela::Render
{

std::expected<DataBufferPtr, bool>
CPUDataBuffer::Create(const IBuffersPtr& buffers,
                      VkBufferUsageFlags vkUsageFlags,
                      const size_t& initialCapacity,
                      const std::string& tag)
{
    const auto bufferCreate = buffers->CreateBuffer(
        vkUsageFlags,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        initialCapacity,
        tag
    );
    if (!bufferCreate.has_value())
    {
        return std::unexpected(false);
    }

    return std::make_shared<CPUDataBuffer>(buffers, bufferCreate.value(), 0);
}

CPUDataBuffer::CPUDataBuffer(IBuffersPtr buffers, BufferPtr buffer, const std::size_t& initialByteSize)
    : DataBuffer(std::move(buffers), std::move(buffer), initialByteSize)
{

}

bool CPUDataBuffer::PushBack(const ExecutionContext& context, const BufferAppend& bufferAppend)
{
    assert(context.type == ExecutionContext::Type::CPU);
    if (context.type != ExecutionContext::Type::CPU) { return false; }

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

bool CPUDataBuffer::Update(const ExecutionContext& context, const std::vector<BufferUpdate>& bufferUpdates)
{
    assert(context.type == ExecutionContext::Type::CPU);
    if (context.type != ExecutionContext::Type::CPU) { return false; }

    return m_buffers->MappedUpdateBuffer(m_buffer, bufferUpdates);
}

bool CPUDataBuffer::Delete(const ExecutionContext& context, const std::vector<BufferDelete>& bufferDeletes)
{
    assert(context.type == ExecutionContext::Type::CPU);
    if (context.type != ExecutionContext::Type::CPU) { return false; }

    if (bufferDeletes.empty()) { return true; }

    //
    // Delete the data sections
    //
    std::size_t totalBytesToDelete{0};

    for (const auto& bufferDelete : bufferDeletes)
    {
        totalBytesToDelete += bufferDelete.deleteByteSize;
    }

    if (!m_buffers->MappedDeleteData(m_buffer, bufferDeletes))
    {
        return false;
    }

    //
    // Resize the buffer down to its new size
    //
    return Resize(context, m_dataByteSize - totalBytesToDelete);
}

bool CPUDataBuffer::Resize(const ExecutionContext& context, const std::size_t& byteSize)
{
    assert(context.type == ExecutionContext::Type::CPU);
    if (context.type != ExecutionContext::Type::CPU) { return false; }

    // Ensure we have enough capacity in the buffer for the new size
    if (!Reserve(context, byteSize))
    {
        return false;
    }

    // Update our size
    m_dataByteSize = byteSize;

    // If our size is <= a quarter of our capacity, cut our capacity in half. Keep
    // a minimum of 16 capacity at all times though, don't let capacity drop to 0.
    if (m_dataByteSize <= m_buffer->GetByteSize() / 4 && m_dataByteSize > 16)
    {
        ResizeBuffer(m_buffer->GetByteSize() / 2);
    }

    return true;
}

bool CPUDataBuffer::Reserve(const ExecutionContext& context, const std::size_t& byteSize)
{
    assert(context.type == ExecutionContext::Type::CPU);
    if (context.type != ExecutionContext::Type::CPU) { return false; }

    if (m_buffer->GetByteSize() >= byteSize)
    {
        return true;
    }

    return ResizeBuffer(byteSize * 2);
}

bool CPUDataBuffer::ResizeBuffer(const std::size_t& newByteSize)
{
    //
    // Create the new buffer
    //
    const auto newBufferCreate = m_buffers->CreateBuffer(
        m_buffer->GetUsageFlags(),
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        newByteSize,
        m_buffer->GetTag()
    );
    if (!newBufferCreate.has_value())
    {
        return false;
    }

    //
    // Copy data from the old buffer into the new buffer
    //
    if (m_dataByteSize > 0)
    {
        const auto bytesToCopy = std::min(m_dataByteSize, newByteSize);

        if (!m_buffers->MappedCopyBufferData(m_buffer, 0, bytesToCopy, *newBufferCreate, 0))
        {
            return false;
        }
    }

    //
    // Delete the old buffer
    //
    m_buffers->DestroyBuffer(m_buffer->GetBufferId());

    //
    // Update internal state
    //
    m_buffer = *newBufferCreate;

    return true;
}

}
