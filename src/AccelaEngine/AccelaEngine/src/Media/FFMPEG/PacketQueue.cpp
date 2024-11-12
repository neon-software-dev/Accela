/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PacketQueue.h"

namespace Accela::Engine
{

bool PacketQueue::PushPacket(AVPacket *pPacket)
{
    AVPacket* pQueuePacket = av_packet_clone(pPacket);
    if (!pQueuePacket)
    {
        return false;
    }

    std::lock_guard m_packetsLock(m_packetsMutex);
    m_packets.push_back(pQueuePacket);
    m_dataByteSize += pQueuePacket->size;

    return true;
}

void PacketQueue::ReturnPacket(AVPacket* pPacket)
{
    std::lock_guard m_packetsLock(m_packetsMutex);
    m_packets.push_front(pPacket);
    m_dataByteSize += pPacket->size;
}

std::optional<AVPacket*> PacketQueue::PopPacket()
{
    std::lock_guard m_packetsLock(m_packetsMutex);

    if (m_packets.empty())
    {
        return std::nullopt;
    }

    auto queuePacket = m_packets.front();
    m_packets.pop_front();
    m_dataByteSize -= queuePacket->size;

    return queuePacket;
}

std::size_t PacketQueue::GetPacketCount() const
{
    return m_packets.size();
}

std::size_t PacketQueue::GetDataByteSize() const
{
    return m_dataByteSize;
}

bool PacketQueue::IsEmpty() const
{
    return m_packets.empty();
}

void PacketQueue::Flush()
{
    std::lock_guard m_packetsLock(m_packetsMutex);

    while (!m_packets.empty())
    {
        auto& queuePacket = m_packets.front();
        av_packet_free(&queuePacket);
        m_packets.pop_front();
    }

    m_dataByteSize = 0;
}

void PacketQueue::Destroy()
{
    Flush();
}

}