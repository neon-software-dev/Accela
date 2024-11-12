/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETQUEUE_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETQUEUE_H

#include "FFMPEGCommon.h"

extern "C"
{
    #include <libavcodec/packet.h>
}

#include <deque>
#include <mutex>
#include <optional>

namespace Accela::Engine
{
    class PacketQueue
    {
        public:

            /**
             * Pushes *a copy of* an AVPacket onto the back of the packet queue. The provided packet
             * is left untouched and the caller retains ownership over it.
             *
             * @param pPacket The packet to push into the queue
             *
             * @return Whether the packet was pushed successfully
             */
            [[nodiscard]] bool PushPacket(AVPacket* pPacket);

            /**
             * Pushes the provided packet onto the front of the packet queue. Used to return a
             * previously popped front packet back into its front position. The queue takes
             * ownership over the packet.
             *
             * @param pPacket The packet to push back onto the front of the queue
             *
             * @return Whether the packet was pushed successfully
             */
            void ReturnPacket(AVPacket* pPacket);

            /**
             * Pops a packet from the queue. The caller owns the popped packet and is responsible
             * for unref'ing its data and freeing the packet itself when no longer needed. Returns
             * std::nullopt if the queue is empty.
             */
            [[nodiscard]] std::optional<AVPacket*> PopPacket();

            /**
             * @return The number of packets in the queue
             */
            [[nodiscard]] std::size_t GetPacketCount() const;

            /**
             * @return The total data byte size associated with the packets in the queue
             */
            [[nodiscard]] std::size_t GetDataByteSize() const;

            /**
             * @return Whether there's no packets in the queue
             */
            [[nodiscard]] bool IsEmpty() const;

            /**
             * Flushes out all currently enqueued packets (freeing their memory in
             * the process).
             */
            void Flush();

            /**
             * Resets the state of the queue to default state
             */
            void Destroy();

        private:

            std::deque<AVPacket*> m_packets;
            std::mutex m_packetsMutex;

            std::size_t m_dataByteSize{0};
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETQUEUE_H
