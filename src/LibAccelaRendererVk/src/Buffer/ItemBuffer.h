/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_BUFFER_ITEMBUFFER_H
#define LIBACCELARENDERERVK_SRC_BUFFER_ITEMBUFFER_H

#include "DataBuffer.h"

#include "../Util/ExecutionContext.h"

namespace Accela::Render
{
    template <typename T>
    class ItemBuffer
    {
        public:

            virtual ~ItemBuffer() = default;

        public:

            ItemBuffer(DataBufferPtr dataBuffer, const std::size_t& size)
                : m_dataBuffer(std::move(dataBuffer))
                , m_size(size)
            { }

            [[nodiscard]] BufferPtr GetBuffer() const noexcept { return m_dataBuffer->GetBuffer(); }
            [[nodiscard]] std::size_t GetSize() const noexcept { return m_size; }

            bool PushBack(const ExecutionContext& context, const std::vector<T>& items)
            {
                BufferAppend bufferAppend{};
                bufferAppend.pData = items.data();
                bufferAppend.dataByteSize = items.size() * sizeof(T);

                if (!m_dataBuffer->PushBack(context, bufferAppend))
                {
                    return false;
                }

                m_size += items.size();

                return true;
            }

            // TODO Perf:: Sort updates by position, combine adjacent updates into one buffer update?
            // TODO Perf:: Update whole buffer if more than X% of items need update?
            bool Update(const ExecutionContext& context, const std::vector<ItemUpdate<T>>& updates)
            {
                std::vector<BufferUpdate> bufferUpdates(updates.size());

                BufferUpdate bufferUpdate;

                std::transform(updates.cbegin(), updates.cend(), bufferUpdates.begin(),
                   [&](const ItemUpdate<T>& itemUpdate){
                       bufferUpdate.pData = &itemUpdate.item;
                       bufferUpdate.dataByteSize = sizeof(T);
                       bufferUpdate.updateOffset = itemUpdate.position * sizeof(T);

                       return bufferUpdate;
                   }
                );

                return m_dataBuffer->Update(context, bufferUpdates);
            }

            bool Update(const ExecutionContext& context, const std::size_t& startPosition, const std::vector<T>& items)
            {
                BufferUpdate bufferUpdate{};
                bufferUpdate.pData = items.data();
                bufferUpdate.dataByteSize = items.size() * sizeof(T);
                bufferUpdate.updateOffset = startPosition * sizeof(T);

                return m_dataBuffer->Update(context, {bufferUpdate});
            }

            bool Resize(const ExecutionContext& context, const std::size_t& size)
            {
                if (!m_dataBuffer->Resize(context, size * sizeof(T)))
                {
                    return false;
                }

                m_size = size;

                return true;
            }

            bool Reserve(const ExecutionContext& context, const std::size_t& size)
            {
                return m_dataBuffer->Reserve(context, size * sizeof(T));
            }

        protected:

            DataBufferPtr m_dataBuffer;
            std::size_t m_size{0};
    };
}

#endif //LIBACCELARENDERERVK_SRC_BUFFER_ITEMBUFFER_H
