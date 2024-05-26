#ifndef LIBACCELARENDERERVK_SRC_BUFFER_DATABUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_DATABUFFER_H

#include "Buffer.h"

#include "../ForwardDeclares.h"

#include "../Util/ExecutionContext.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <vector>
#include <string>

namespace Accela::Render
{
    class DataBuffer
    {
        public:

            virtual ~DataBuffer() = default;

            [[nodiscard]] BufferPtr GetBuffer() const noexcept { return m_buffer; }
            [[nodiscard]] std::size_t GetDataByteSize() const noexcept { return m_dataByteSize; }

            virtual bool PushBack(const ExecutionContext& context, const BufferAppend& bufferAppend) = 0;
            virtual bool Update(const ExecutionContext& context, const std::vector<BufferUpdate>& bufferUpdates) = 0;
            virtual bool Delete(const ExecutionContext& context, const std::vector<BufferDelete>& bufferDeletes) = 0;
            virtual bool Resize(const ExecutionContext& context, const std::size_t& byteSize) = 0;
            virtual bool Reserve(const ExecutionContext& context, const std::size_t& byteSize) = 0;

        protected:

            DataBuffer(IBuffersPtr buffers, BufferPtr buffer, const std::size_t& initialByteSize);

        protected:

            IBuffersPtr m_buffers;
            BufferPtr m_buffer;

            // The byte count of actual data used within the buffer's allocated capacity/size
            std::size_t m_dataByteSize{0};
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_DATABUFFER_H
