/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETREADER_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETREADER_H

#include "PacketQueue.h"

#include "../MediaCommon.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <memory>
#include <future>

namespace Accela::Engine
{
    class FFMPEGContainer;

    template <typename FrameType>
    class PacketDecoder;

    class SubtitleDecoder;

    class PacketReader
    {
        public:

            PacketReader(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics, FFMPEGContainer* pContainer);
            ~PacketReader();

            void SetDecoders(PacketDecoder<VideoFrame>* pVideoDecoder, PacketDecoder<AudioFrame>* pAudioDecoder);

            void Start();
            void DoStockPackets();
            [[nodiscard]] std::future<bool> StopWork();
            void ResumeWork();

            void FlushPackets();
            [[nodiscard]] PacketQueue& VideoPacketQueue() { return m_videoPacketQueue; }
            [[nodiscard]] PacketQueue& AudioPacketQueue() { return m_audioPacketQueue; }
            [[nodiscard]] PacketQueue& SubtitlePacketQueue() { return m_subtitlePacketQueue; }

        private:

            enum class State
            {
                Stocking,
                Stopped
            };

        private:

            void MessageHandler(const Common::Message::Ptr& message);

            void Thread_StockPackets();
            [[nodiscard]] bool Thread_QueuesNeedMorePackets() const;
            [[nodiscard]] bool Thread_StockPacket();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            FFMPEGContainer* m_pContainer;

            State m_state{State::Stocking};
            PacketDecoder<VideoFrame>* m_pVideoDecoder{nullptr};
            PacketDecoder<AudioFrame>* m_pAudioDecoder{nullptr};

            PacketQueue m_videoPacketQueue;
            PacketQueue m_audioPacketQueue;
            PacketQueue m_subtitlePacketQueue;

            std::unique_ptr<Common::MessageDrivenThreadPool> m_thread;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETREADER_H
