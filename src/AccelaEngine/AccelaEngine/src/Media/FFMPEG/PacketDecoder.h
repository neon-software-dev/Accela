/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETDECODER_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETDECODER_H

#include "FFMPEGCommon.h"
#include "PacketReader.h"
#include "FFMPEGContainer.h"

#include "../MediaCommon.h"

#include <Accela/Common/Timer.h>
#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>
#include <Accela/Common/Thread/ResultMessage.h>

#include <memory>
#include <future>
#include <deque>
#include <optional>
#include <string>

namespace Accela::Engine
{
    template <typename FrameType>
    class PacketDecoder
    {
        public:

            enum class State
            {
                Decoding,
                Flushing,
                Flushed,
                Stopped
            };

            static constexpr auto IDLE_PACKET_DECODE_INTERVAL = std::chrono::milliseconds(10);

            static constexpr auto DECODE_PACKETS_MESSAGE = "DecodePackets";
            static constexpr auto STOP_WORK_MESSAGE = "StopWork";
            static constexpr auto RESUME_WORK_MESSAGE = "ResumeWork";

        public:

            PacketDecoder(Common::ILogger::Ptr logger,
                          Common::IMetrics::Ptr metrics,
                          MediaStreamType mediaStreamType,
                          unsigned int minDecodedFrameCount,
                          FFMPEGContainer* pContainer)
                : m_logger(std::move(logger))
                , m_metrics(std::move(metrics))
                , m_mediaStreamType(mediaStreamType)
                , m_minDecodedFrameCount(minDecodedFrameCount)
                , m_pContainer(pContainer)
            {

            }

            ~PacketDecoder()
            {
                // Stops and joins the thread pool
                m_thread = nullptr;

                m_pPacketReader = nullptr;
                m_pContainer = nullptr;
            }

            void SetReader(PacketReader* pPacketReader)
            {
                m_pPacketReader = pPacketReader;
            }

            void Start()
            {
                m_thread = std::make_unique<Common::MessageDrivenThreadPool>(
                    TagForMediaStreamType(m_mediaStreamType),
                    1,
                    [this](const Common::Message::Ptr& message) { MessageHandler(message); },
                    [this]() { Thread_DecodePackets(); },
                    IDLE_PACKET_DECODE_INTERVAL
                );
            }

            void OnPacketsStocked()
            {
                m_thread->PostMessage(DECODE_PACKETS_MESSAGE);
            }

            [[nodiscard]] std::future<bool> StopWork()
            {
                auto message = std::make_shared<Common::ResultMessage<bool>>(STOP_WORK_MESSAGE);
                auto messageFuture = message->CreateFuture();
                m_thread->PostMessage(message);
                return messageFuture;
            }

            void ResumeWork()
            {
                m_thread->PostMessage(RESUME_WORK_MESSAGE);
            }

            void FlushFrames()
            {
                std::lock_guard m_decodedFrameQueueLock(m_decodedFrameQueueMutex);

                m_decodedFrameQueue.clear();
            }

            [[nodiscard]] State GetState() const { return m_state; }
            [[nodiscard]] std::size_t GetDecodedFrameQueueSize() const { return m_decodedFrameQueue.size(); }

            [[nodiscard]] std::optional<FrameType> PeekFrontFrame() const
            {
                std::lock_guard m_decodedFrameQueueLock(m_decodedFrameQueueMutex);

                if (m_decodedFrameQueue.empty())
                {
                    return std::nullopt;
                }

                return m_decodedFrameQueue.front();
            }

            [[nodiscard]] std::optional<FrameType> PopFrontFrame()
            {
                std::lock_guard m_decodedFrameQueueLock(m_decodedFrameQueueMutex);

                if (m_decodedFrameQueue.empty())
                {
                    return std::nullopt;
                }

                auto frame = m_decodedFrameQueue.front();

                m_decodedFrameQueue.pop_front();

                m_thread->PostMessage(DECODE_PACKETS_MESSAGE);

                return frame;
            }

        private:

            void MessageHandler(const Common::Message::Ptr& message)
            {
                if (message->GetTypeIdentifier() == DECODE_PACKETS_MESSAGE)
                {
                    Thread_DecodePackets();
                }
                else if (message->GetTypeIdentifier() == STOP_WORK_MESSAGE)
                {
                    m_state = State::Stopped;
                    std::dynamic_pointer_cast<Common::ResultMessage<bool>>(message)->SetResult(true);
                }
                else if (message->GetTypeIdentifier() == RESUME_WORK_MESSAGE)
                {
                    m_state = State::Decoding;
                }
            }

            void Thread_DecodePackets()
            {
                auto* pPacketQueue = GetPacketQueue();
                if (pPacketQueue == nullptr) { return; }

                // Don't decode packets if we're in stopped or flushed state
                if (m_state == State::Stopped || m_state == State::Flushed)
                {
                    return;
                }

                //
                // If we already have enough frames decoded, we can bail out now
                //
                if (m_decodedFrameQueue.size() >= m_minDecodedFrameCount)
                {
                    return;
                }

                // If we're in decoding state and see that the container is at EOF, move into the Flushing state
                if (m_state == State::Decoding && m_pContainer->IsEOF())
                {
                    m_state = State::Flushing;
                }

                // If we're in Flushing state and there's no packets left in the queue to be sent to the decoder, tell the
                // container to flush the decoder, which will allow any remaining frames still in it to come out
                if (m_state == State::Flushing && pPacketQueue->IsEmpty())
                {
                    m_pContainer->FlushDecoder(m_mediaStreamType);
                }

                //
                // Fill the decoder with as many packets as we can
                //
                const auto numPacketsSent = Thread_FillDecoder();

                //
                // Consume as many frames from the decoder as we can
                //
                const auto numFramesDecoded = Thread_ReceiveDecodedFrames();

                //
                // If we sent any packets to the decoder, tell the packet read thread that it should
                // look into stocking the packet queue back up
                //
                if (numPacketsSent > 0)
                {
                    m_pPacketReader->DoStockPackets();
                }

                //
                // If we're in flushing state, and we got no more frames back from the decoder, and we have no frames
                // left in our queue, consider us flushed
                //
                if (m_state == State::Flushing && numFramesDecoded == 0 && m_decodedFrameQueue.empty())
                {
                    m_state = State::Flushed;
                }

                //
                // Finish by recording metrics
                //
                m_metrics->SetCounterValue(std::format("DECODER_QUEUE_COUNT_{}", TagForMediaStreamType(m_mediaStreamType)), m_decodedFrameQueue.size());
            }

            [[nodiscard]] std::vector<FrameType> Thread_ReceiveFramesFromDecoder();

            unsigned int Thread_ReceiveDecodedFrames()
            {
                unsigned int numFramesDecoded = 0;

                while (true)
                {
                    const auto frames = Thread_ReceiveFramesFromDecoder();
                    if (frames.empty())
                    {
                        return numFramesDecoded;
                    }

                    std::lock_guard m_decodedFrameQueueLock(m_decodedFrameQueueMutex);
                    std::ranges::copy(frames, std::back_inserter(m_decodedFrameQueue));
                    numFramesDecoded += frames.size();
                }
            }

            [[nodiscard]] unsigned int Thread_FillDecoder()
            {
                auto* pPacketQueue = GetPacketQueue();

                if (pPacketQueue == nullptr) { return 0; }

                unsigned int sentPacketCount = 0;

                while (true)
                {
                    // Pop the next packet from the packet queue
                    const auto poppedPacket = pPacketQueue->PopPacket();

                    // If the packet queue was empty, nothing more to do
                    if (!poppedPacket)
                    {
                        return sentPacketCount;
                    }

                    // Otherwise, send the packet to the codec for decoding
                    const auto decodeException = m_pContainer->SendPacketToDecoder(m_mediaStreamType, *poppedPacket);

                    // If the decoder was full, return the packet back to the packet queue and
                    // then there's nothing more we can do since the decoder isn't accepting
                    // more packets right now
                    if (decodeException == FFMPEGContainer::SendPacketException::Full)
                    {
                        pPacketQueue->ReturnPacket(*poppedPacket);
                        return sentPacketCount;
                    }

                    sentPacketCount++;

                    // Otherwise, on decode error or decode success, free the popped packet and loop again and try to pop+decode another
                    AVPacket *pPacket = *poppedPacket;
                    av_packet_free(&pPacket);
                }
            }

            [[nodiscard]] PacketQueue* GetPacketQueue() const
            {
                switch (m_mediaStreamType)
                {
                    case MediaStreamType::Video: return &m_pPacketReader->VideoPacketQueue();
                    case MediaStreamType::Audio: return &m_pPacketReader->AudioPacketQueue();
                    default: return nullptr;
                }

                assert(false);
                return nullptr;
            }

            [[nodiscard]] static std::string TagForMediaStreamType(MediaStreamType mediaStreamType)
            {
                switch (mediaStreamType)
                {
                    case MediaStreamType::Video: return "VideoDecoder";
                    case MediaStreamType::Audio: return "AudioDecoder";
                    default: return "Unsupported";
                }

                assert(false);
                return "Unknown";
            }

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            MediaStreamType m_mediaStreamType;
            unsigned int m_minDecodedFrameCount;
            FFMPEGContainer* m_pContainer;

            State m_state{State::Decoding};
            PacketReader* m_pPacketReader{nullptr};

            mutable std::mutex m_decodedFrameQueueMutex;
            std::deque<FrameType> m_decodedFrameQueue;

            std::unique_ptr<Common::MessageDrivenThreadPool> m_thread;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_FFMPEG_PACKETDECODER_H
