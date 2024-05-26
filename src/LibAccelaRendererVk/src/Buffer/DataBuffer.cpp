#include "DataBuffer.h"

namespace Accela::Render
{

DataBuffer::DataBuffer(IBuffersPtr buffers, BufferPtr buffer, const std::size_t& initialByteSize)
    : m_buffers(std::move(buffers))
    , m_buffer(std::move(buffer))
    , m_dataByteSize(initialByteSize)
{

}

}
