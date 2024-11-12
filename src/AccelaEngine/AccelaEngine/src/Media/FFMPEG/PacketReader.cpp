/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PacketReader.h"
#include "PacketDecoder.h"
#include "SubtitleDecoder.h"
#include "FFMPEGContainer.h"

#include <Accela/Common/Thread/ResultMessage.h>

#include <vector>

namespace Accela::Engine
{

static constexpr unsigned int MAX_TOTAL_QUEUE_BYTE_SIZE = 30 * 1024 * 1024; // 30MB
static constexpr unsigned int MIN_VIDEO_PACKET_COUNT = 16; // Packets
static constexpr unsigned int MIN_AUDIO_PACKET_COUNT = 16; // Packets
static constexpr unsigned int MIN_SUBTITLE_PACKET_COUNT = 2; // Packets

static constexpr auto IDLE_PACKET_READ_INTERVAL = std::chrono::milliseconds(10);

static constexpr auto STOCK_PACKETS_MESSAGE = "StockPackets";
static constexpr auto STOP_WORK_MESSAGE = "StopWork";
static constexpr auto RESUME_WORK_MESSAGE = "ResumeWork";

PacketReader::PacketReader(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           FFMPEGContainer* pContainer)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_pContainer(pContainer)
{

}

PacketReader::~PacketReader()
{
    // Stops and joins the thread pool
    m_thread = nullptr;

    m_videoPacketQueue.Destroy();
    m_audioPacketQueue.Destroy();
    m_subtitlePacketQueue.Destroy();

    m_pVideoDecoder = nullptr;
    m_pAudioDecoder = nullptr;
    m_pContainer = nullptr;
}

void PacketReader::SetDecoders(PacketDecoder<VideoFrame>* pVideoDecoder, PacketDecoder<AudioFrame>* pAudioDecoder)
{
    m_pVideoDecoder = pVideoDecoder;
    m_pAudioDecoder = pAudioDecoder;
}

void PacketReader::Start()
{
    m_thread = std::make_unique<Common::MessageDrivenThreadPool>(
        "PacketReader",
        1,
        [this](const Common::Message::Ptr& message) { MessageHandler(message); },
        [this]() { Thread_StockPackets(); },
        IDLE_PACKET_READ_INTERVAL
    );
}

void PacketReader::DoStockPackets()
{
    m_thread->PostMessage(STOCK_PACKETS_MESSAGE);
}

std::future<bool> PacketReader::StopWork()
{
    auto message = std::make_shared<Common::ResultMessage<bool>>(STOP_WORK_MESSAGE);
    auto messageFuture = message->CreateFuture();
    m_thread->PostMessage(message);
    return messageFuture;
}

void PacketReader::ResumeWork()
{
    m_thread->PostMessage(RESUME_WORK_MESSAGE);
}

void PacketReader::FlushPackets()
{
    m_videoPacketQueue.Flush();
    m_audioPacketQueue.Flush();
}

void PacketReader::MessageHandler(const Common::Message::Ptr& message)
{
    if (message->GetTypeIdentifier() == STOCK_PACKETS_MESSAGE)
    {
        Thread_StockPackets();
    }
    else if (message->GetTypeIdentifier() == STOP_WORK_MESSAGE)
    {
        m_state = State::Stopped;
        std::dynamic_pointer_cast<Common::ResultMessage<bool>>(message)->SetResult(true);
    }
    else if (message->GetTypeIdentifier() == RESUME_WORK_MESSAGE)
    {
        m_state = State::Stocking;
        Thread_StockPackets();
    }
}

void PacketReader::Thread_StockPackets()
{
    // Don't stock packets if we're not in packet stocking state
    if (m_state != State::Stocking)
    {
        return;
    }

    // Don't stock packets if the container is in EOF state and has no more packets
    if (m_pContainer->IsEOF())
    {
        return;
    }

    // Otherwise, as long as our queues still need packets, stock packets in them
    while (Thread_QueuesNeedMorePackets())
    {
        if (!Thread_StockPacket())
        {
            break;
        }
    }

    // Finish by updating metrics
    m_metrics->SetCounterValue("VIDEO_PACKET_QUEUE_COUNT", m_videoPacketQueue.GetPacketCount());
    m_metrics->SetCounterValue("AUDIO_PACKET_QUEUE_COUNT", m_audioPacketQueue.GetPacketCount());
}

bool PacketReader::Thread_QueuesNeedMorePackets() const
{
    //
    // Adhere to a limit on the total byte size of queued packets
    //
    const auto totalQueueByteSize =
        m_videoPacketQueue.GetDataByteSize() +
        m_audioPacketQueue.GetDataByteSize() +
        m_subtitlePacketQueue.GetDataByteSize();

    if (totalQueueByteSize >= MAX_TOTAL_QUEUE_BYTE_SIZE)
    {
        return false;
    }

    //
    // Don't need more packets if all of our queues are at the minimum packet count
    //
    if (m_videoPacketQueue.GetPacketCount() >= MIN_VIDEO_PACKET_COUNT &&
        m_audioPacketQueue.GetPacketCount() >= MIN_AUDIO_PACKET_COUNT)
    {
        return false;
    }

    return true;
}

bool PacketReader::Thread_StockPacket()
{
    const auto packetRead = m_pContainer->ReadPacket();
    if (!packetRead)
    {
        switch (packetRead.error())
        {
            case FFMPEGContainer::ReadException::Error:
            {
                LogError("FFMPEGMediaSource::PacketReadThread_StockPackets: Error reading next container packet");
                return false;
            }
            case FFMPEGContainer::ReadException::Eof:
            {
                // Just bail out if we've newly hit eof on that packet read
                return false;
            }
        }
    }

    //
    // Push the read packet into the appropriate packet queue
    //
    if ((*packetRead)->stream_index == m_pContainer->GetVideoStreamIndex())
    {
        //LogError("TEMP: Read video packet: {}", (*packetRead)->pts * (1.0 / 60000.0));

        // Push the packet into the video packet queue
        if (!m_videoPacketQueue.PushPacket(*packetRead))
        {
            LogError("PacketReader::Thread_StockPacket: Failed to enqueue video packet");
        }

        m_metrics->SetCounterValue("VIDEO_PACKET_QUEUE_COUNT", m_videoPacketQueue.GetPacketCount());

        // Tell the video decoder that there's more data in the video packet queue
        m_pVideoDecoder->OnPacketsStocked();
    }
    else if ((*packetRead)->stream_index == m_pContainer->GetAudioStreamIndex())
    {
        //LogError("TEMP: Read audio packet: {}", (*packetRead)->pts * (1.0 / 48000.0));

        // Push the packet into the audio packet queue
        if (!m_audioPacketQueue.PushPacket(*packetRead))
        {
            LogError("FFMPEGMediaSource::PacketReadThread_StockPackets: Failed to enqueue audio packet");
        }

        m_metrics->SetCounterValue("AUDIO_PACKET_QUEUE_COUNT", m_audioPacketQueue.GetPacketCount());

        // Tell the audio decoder that there's more data in the video packet queue
        m_pAudioDecoder->OnPacketsStocked();
    }
    /*else if ((*packetRead)->stream_index == m_pContainer->GetSubtitleStreamIndex())
    {
        if (!m_subtitlePacketQueue.PushPacket(*packetRead))
        {
            LogError("FFMPEGMediaSource::PacketReadThread_StockPackets: Failed to enqueue subtitle packet");
        }

        m_metrics->SetCounterValue("SUBTITLE_PACKET_QUEUE_COUNT", m_subtitlePacketQueue.GetPacketCount());

        // Tell the subtitle decoder that there's more data in the video packet queue
        m_pSubtitleDecoder->OnPacketsStocked();
    }*/

    // Whether it was successfully pushed (cloned) into a queue or not, mark the packet as no longer referencing
    // its buffer, but leave the packet itself allocated as it's the working packet that the container owns
    av_packet_unref(*packetRead);

    return true;
}

}
