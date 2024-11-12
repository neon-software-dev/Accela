/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FFMPEGMediaSource.h"

#include <Accela/Common/Timer.h>

namespace Accela::Engine
{

static constexpr unsigned int MIN_DECODED_VIDEO_FRAME_COUNT = 16; // Frames
static constexpr unsigned int MIN_DECODED_AUDIO_FRAME_COUNT = 48; // Frames
static constexpr unsigned int MIN_DECODED_SUBTITLE_FRAME_COUNT = 2; // Frames

FFMPEGMediaSource::FFMPEGMediaSource(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics, FFMPEGContainer::UPtr container)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_container(std::move(container))
    , m_packetReader(std::make_unique<PacketReader>(m_logger, m_metrics, m_container.get()))
    , m_videoDecoder(std::make_unique<PacketDecoder<VideoFrame>>(m_logger, m_metrics, MediaStreamType::Video, MIN_DECODED_VIDEO_FRAME_COUNT, m_container.get()))
    , m_audioDecoder(std::make_unique<PacketDecoder<AudioFrame>>(m_logger, m_metrics, MediaStreamType::Audio, MIN_DECODED_AUDIO_FRAME_COUNT, m_container.get()))
{
    m_packetReader->SetDecoders(m_videoDecoder.get(), m_audioDecoder.get());
    m_videoDecoder->SetReader(m_packetReader.get());
    m_audioDecoder->SetReader(m_packetReader.get());

    m_packetReader->Start();

    m_videoDecoder->Start();
    m_audioDecoder->Start();
}

std::size_t FFMPEGMediaSource::GetVideoFrameQueueSize() const
{
    return m_videoDecoder->GetDecodedFrameQueueSize();
}

std::optional<VideoFrame> FFMPEGMediaSource::PeekFrontVideoFrame() const
{
    return m_videoDecoder->PeekFrontFrame();
}

std::optional<VideoFrame> FFMPEGMediaSource::PopFrontVideoFrame()
{
   return m_videoDecoder->PopFrontFrame();
}

std::size_t FFMPEGMediaSource::GetAudioFrameQueueSize() const
{
    return m_audioDecoder->GetDecodedFrameQueueSize();
}

std::optional<AudioFrame> FFMPEGMediaSource::PeekFrontAudioFrame() const
{
    return m_audioDecoder->PeekFrontFrame();
}

std::optional<AudioFrame> FFMPEGMediaSource::PopFrontAudioFrame()
{
    return m_audioDecoder->PopFrontFrame();
}

MediaDuration FFMPEGMediaSource::GetSourceDuration() const
{
    return m_container->GetSourceDuration();
}

bool FFMPEGMediaSource::HasHitEnd() const
{
    // The media source is considered to have hit the end of its content when the container has
    // reached EOF and both the video and audio decoders have reached flushed state
    return m_container->IsEOF() &&
            (m_videoDecoder->GetState() == PacketDecoder<VideoFrame>::State::Flushed &&
             m_audioDecoder->GetState() == PacketDecoder<AudioFrame>::State::Flushed);
}

bool FFMPEGMediaSource::LoadFromPoint(MediaPoint point, const std::optional<MediaDuration>& loadOffset)
{
    // Stop worker threads and ditch all enqueued/working data
    Stop();

    // Seek the container to the point
    const bool seekSuccess = m_container->SeekToPoint(point, loadOffset);

    // Tell the workers to resume fetching and decoding
    m_packetReader->ResumeWork();
    m_videoDecoder->ResumeWork();
    m_audioDecoder->ResumeWork();

    return seekSuccess;
}

bool FFMPEGMediaSource::LoadStreams(MediaPoint curPoint, const std::unordered_set<unsigned int>& streamIndices)
{
    // Stop worker threads and ditch all enqueued/working data
    Stop();

    // Tell the container to load the streams, and re-seek to the current point, which will seek to
    // a stream location nearby that the new stream configuration can be played from without artifacts
    bool success = m_container->LoadStreams(streamIndices) && m_container->SeekToPoint(curPoint);

    // Tell the workers to resume fetching and decoding
    m_packetReader->ResumeWork();
    m_videoDecoder->ResumeWork();
    m_audioDecoder->ResumeWork();

    return success;
}

void FFMPEGMediaSource::SetAudioSyncDiff(const MediaDuration& audioSyncDiff)
{
    m_container->SetAudioSyncDiff(audioSyncDiff);
}

void FFMPEGMediaSource::Stop()
{
    // Stop worker threads from fetching and decoding packets
    m_videoDecoder->StopWork().get();
    m_audioDecoder->StopWork().get();
    m_packetReader->StopWork().get();

    // Abandon any data in the ffmpeg decoders
    m_container->FlushDecoder(MediaStreamType::Video);
    m_container->FlushDecoder(MediaStreamType::Audio);

    // Flush all queued packets
    m_packetReader->FlushPackets();

    // Flush all decoded frames
    m_videoDecoder->FlushFrames();
    m_audioDecoder->FlushFrames();

    // Reset audio sync diff
    m_container->SetAudioSyncDiff(MediaDuration(0.0));
}

void FFMPEGMediaSource::Destroy()
{
    // Stop and flush the worker threads
    Stop();

    // Join the worker threads
    m_videoDecoder = nullptr;
    m_audioDecoder = nullptr;
    m_packetReader = nullptr;

    // Destroy any data in the FFMPEG container
    m_container->Destroy();
}

}
