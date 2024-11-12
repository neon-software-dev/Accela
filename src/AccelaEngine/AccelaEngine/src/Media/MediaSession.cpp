/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MediaSession.h"

#include "../Audio/AudioManager.h"
#include "../Audio/AudioUtil.h"

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

// How long the session thread loop will sleep for if it's unable to determine when the next
// audio or video presentation point is
static constexpr auto DEFAULT_RUN_SLEEP = MediaDuration(0.01);

// How many presented audio frame sync calculations are used to calculate/report current audio sync
static constexpr auto AUDIO_SYNC_SAMPLE_SIZE = 20;

static constexpr auto PLAY_COMMAND = "PlayCommand";
static constexpr auto PAUSE_COMMAND = "PauseCommand";
static constexpr auto STOP_COMMAND = "StopCommand";
static constexpr auto SEEK_BY_OFFSET_COMMAND = "SeekByOffsetCommand";
static constexpr auto SEEK_TO_POINT_COMMAND = "SeekToPointCommand";
static constexpr auto LOAD_STREAMS_COMMAND = "LoadStreamsCommand";

struct PlayCommand : public Common::ResultMessage<bool>
{
    explicit PlayCommand(std::optional<MediaPoint> _playPoint)
        : Common::ResultMessage<bool>(PLAY_COMMAND), playPoint(_playPoint)
    {}

    std::optional<MediaPoint> playPoint;
};

struct SeekByOffsetCommand : public Common::ResultMessage<bool>
{
    explicit SeekByOffsetCommand(MediaDuration _offset)
        : Common::ResultMessage<bool>(SEEK_BY_OFFSET_COMMAND), offset(_offset)
    {}

    MediaDuration offset;
};

struct SeekToPointCommand : public Common::ResultMessage<bool>
{
    explicit SeekToPointCommand(MediaPoint _point)
        : Common::ResultMessage<bool>(SEEK_TO_POINT_COMMAND), point(_point)
    {}

    MediaPoint point;
};

struct LoadStreamsCommand : public Common::ResultMessage<bool>
{
    explicit LoadStreamsCommand(std::unordered_set<unsigned int> _streamIndices)
        : Common::ResultMessage<bool>(LOAD_STREAMS_COMMAND), streamIndices(std::move(_streamIndices))
    {}

    std::unordered_set<unsigned int> streamIndices;
};

MediaSession::MediaSession(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           Render::IRenderer::Ptr renderer,
                           AudioManagerPtr audioManager,
                           MediaSessionId mediaSessionId,
                           IMediaSource::Ptr mediaSource,
                           Common::ImageData::Ptr initialImage,
                           Render::TextureId textureId,
                           AudioSourceId audioSourceId,
                           MasterClockType masterClockType)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_renderer(std::move(renderer))
    , m_audioManager(std::move(audioManager))
    , m_mediaSessionId(mediaSessionId)
    , m_mediaSource(std::move(mediaSource))
    , m_initialImage(std::move(initialImage))
    , m_textureId(textureId)
    , m_audioSourceId(audioSourceId)
    , m_masterClockType(masterClockType)
    , m_sessionThread(std::thread(&MediaSession::ThreadFunc, this))
{

}

void MediaSession::Destroy()
{
    LogInfo("MediaSession: Destroying media session: {}", m_mediaSessionId.id);

    m_doRunSession = false;
    m_sessionThread.join();
}

MediaSessionId MediaSession::GetMediaSessionId() const
{
    return m_mediaSessionId;
}

Render::TextureId MediaSession::GetTextureId() const
{
    return m_textureId;
}

AudioSourceId MediaSession::GetAudioSourceId() const
{
    return m_audioSourceId;
}

std::future<bool> MediaSession::Play(const std::optional<MediaPoint>& initialPoint)
{
    auto message = std::make_shared<PlayCommand>(initialPoint);
    auto messageFuture = message->CreateFuture();
    m_commandQueue.Push(message);
    return messageFuture;
}

std::future<bool> MediaSession::Pause()
{
    auto message = std::make_shared<Common::ResultMessage<bool>>(PAUSE_COMMAND);
    auto messageFuture = message->CreateFuture();
    m_commandQueue.Push(message);
    return messageFuture;
}

std::future<bool> MediaSession::Stop()
{
    auto message = std::make_shared<Common::ResultMessage<bool>>(STOP_COMMAND);
    auto messageFuture = message->CreateFuture();
    m_commandQueue.Push(message);
    return messageFuture;
}

std::future<bool> MediaSession::SeekByOffset(MediaDuration mediaDuration)
{
    auto message = std::make_shared<SeekByOffsetCommand>(mediaDuration);
    auto messageFuture = message->CreateFuture();
    m_commandQueue.Push(message);
    return messageFuture;
}

std::future<bool> MediaSession::SeekToPoint(MediaPoint mediaPoint)
{
    auto message = std::make_shared<SeekToPointCommand>(mediaPoint);
    auto messageFuture = message->CreateFuture();
    m_commandQueue.Push(message);
    return messageFuture;
}

std::future<bool> MediaSession::LoadStreams(const std::unordered_set<unsigned int>& streamIndices)
{
    auto message = std::make_shared<LoadStreamsCommand>(streamIndices);
    auto messageFuture = message->CreateFuture();
    m_commandQueue.Push(message);
    return messageFuture;
}

template<typename T>
std::optional<T> ChooseSmallestIfBothValid(const std::optional<T>& a, const std::optional<T>& b)
{
    if (!a || !b) { return std::nullopt;}
    else { return std::min(*a, *b); }
}

void MediaSession::ThreadFunc()
{
    LogInfo("MediaSession: Media session {} thread running", m_mediaSessionId.id);

    while (m_doRunSession)
    {
        Thread_ProcessCommands();

        if (m_mediaSource->HasHitEnd())
        {
            (void)Thread_StopCommand();
        }

        MediaDuration sleepDuration = DEFAULT_RUN_SLEEP;

        if (m_mediaState == MediaState::Playing || m_mediaState == MediaState::Seeking)
        {
            auto now = std::chrono::steady_clock::now();

            //
            // Consume the frame queues and present video/audio frames as needed
            //
            const bool presentedVideoFrame = Thread_PresentVideoFrame(now);
            Thread_PresentAudioFrame(now);

            //
            // If we're seeking, and we just presented a video frame, then the seek
            // is finished; transition back to the state we were in before seeking
            //
            if ((m_mediaState == MediaState::Seeking) && presentedVideoFrame)
            {
                if (m_seekSourceState == MediaState::Playing)
                {
                    m_mediaState = MediaState::Playing;
                    m_seekSourceState = std::nullopt;
                }
                else if (m_seekSourceState == MediaState::Paused)
                {
                    m_mediaState = MediaState::Paused;
                    m_seekSourceState = std::nullopt;
                    continue; // Don't continue presentation logic below
                }
            }

            //
            // Update our tracking of how out of sync the audio stream is, potentially
            // passing the current offset to the container for it to adjust audio decoding
            //
            Thread_RecordAudioSyncDiff(now);

            //
            // The "next" present point is whatever the closer present point is, between video and audio presentation.
            // If we're missing one of the present points, default to a default sleep interval below.
            //
            const auto nextPresentPoint = ChooseSmallestIfBothValid(m_nextVideoPresentPoint, m_nextAudioPresentPoint);

            // Re-fetch clock time right before calculating a sleep interval, so it's as accurate as possible
            const auto masterClockTime = GetMasterClockMediaPoint(std::chrono::steady_clock::now());

            // If we have a next present point, that's how long we sleep for
            if (nextPresentPoint && masterClockTime)
            {
                if (*nextPresentPoint >= *masterClockTime)
                {
                    sleepDuration = *nextPresentPoint - *masterClockTime;
                }
                else
                {
                    // Theoretically an insanely rare edge case - when the next present point is a tiny distance
                    // in the future, and the logic this method has run since the present point was calculated has
                    // passed that point
                    sleepDuration = MediaDuration(0.0);
                }
            }
        }

        std::this_thread::sleep_for(sleepDuration);
    }
}

void MediaSession::Thread_ProcessCommands()
{
    while (!m_commandQueue.IsEmpty())
    {
        const auto command = m_commandQueue.TryPop();
        if (!command) { return; }

        if ((*command)->GetTypeIdentifier() == PLAY_COMMAND)
        {
            auto playCommand = std::dynamic_pointer_cast<PlayCommand>(*command);
            playCommand->SetResult(Thread_PlayCommand(playCommand->playPoint));
        }
        else if ((*command)->GetTypeIdentifier() == PAUSE_COMMAND)
        {
            auto pauseCommand = std::dynamic_pointer_cast<Common::ResultMessage<bool>>(*command);
            pauseCommand->SetResult(Thread_PauseCommand());
        }
        else if ((*command)->GetTypeIdentifier() == STOP_COMMAND)
        {
            auto stopCommand = std::dynamic_pointer_cast<Common::ResultMessage<bool>>(*command);
            stopCommand->SetResult(Thread_StopCommand());
        }
        else if ((*command)->GetTypeIdentifier() == SEEK_BY_OFFSET_COMMAND)
        {
            auto seekByOffsetCommand = std::dynamic_pointer_cast<SeekByOffsetCommand>(*command);
            seekByOffsetCommand->SetResult(Thread_SeekByOffsetCommand(seekByOffsetCommand->offset));
        }
        else if ((*command)->GetTypeIdentifier() == SEEK_TO_POINT_COMMAND)
        {
            auto seekToPointCommand = std::dynamic_pointer_cast<SeekToPointCommand>(*command);
            seekToPointCommand->SetResult(Thread_SeekToPointCommand(seekToPointCommand->point));
        }
        else if ((*command)->GetTypeIdentifier() == LOAD_STREAMS_COMMAND)
        {
            auto loadStreamsCommand = std::dynamic_pointer_cast<LoadStreamsCommand>(*command);
            loadStreamsCommand->SetResult(Thread_LoadStreamsCommand(loadStreamsCommand->streamIndices));
        }
        else
        {
            LogError("MediaSession::Thread_ProcessCommands: Unsupported command type: {}", (*command)->GetTypeIdentifier());
        }
    }
}

bool MediaSession::Thread_PlayCommand(const std::optional<MediaPoint>& playPoint)
{
    bool playSuccessful = true;

    switch (m_mediaState)
    {
        case MediaState::Playing:
        case MediaState::Seeking:
        {
            // Don't do anything if we're already playing or actively seeking
            return true;
        }
        case MediaState::Paused:
        {
            // When we resume playing, update the clocks' sync times to the resume time; essentially
            // pretends the period of time we were paused didn't exist, and continues on
            const auto resumeTime = std::chrono::steady_clock::now();
            m_externalClock.syncTime = resumeTime;
            m_videoClock.syncTime = resumeTime;
            m_audioClock.syncTime = resumeTime;

            // If we're supposed to be resuming from a specific point, seek to that point now
            if (playPoint)
            {
                playSuccessful = Thread_SeekToPointCommand(*playPoint);
            }

            // Resume the playback of previously enqueued audio
            (void)m_audioManager->PlaySource(m_audioSourceId);
        }
        break;
        case MediaState::Stopped:
        {
            // If a load point was provided, play from that point, otherwise play from the beginning
            playSuccessful = m_mediaSource->LoadFromPoint(playPoint ? *playPoint : MediaPoint(0.0), std::nullopt);
        }
        break;
    }

    m_mediaState = MediaState::Playing;

    return playSuccessful;
}

bool MediaSession::Thread_PauseCommand()
{
    // If we're not in Playing state, pausing doesn't do anything
    if (m_mediaState != MediaState::Playing)
    {
        return true;
    }

    m_mediaState = MediaState::Paused;

    // Sync the external clock to the time we're pausing at so that when we
    // resume it can tick forward from the media point we paused at. The other
    // clock types will update their sync point then next time they process
    // another frame of data after resuming.
    const auto now = std::chrono::steady_clock::now();
    const auto externalClockTime = m_externalClock.InterpolatedTime(now);
    if (externalClockTime)
    {
        m_externalClock.syncPoint = *externalClockTime;
        m_externalClock.syncTime = now;
    }

    // Reset any accumulated audio sync data
    Thread_ResetAudioSyncDiff();

    // Pause playback of enqueued audio data
    return m_audioManager->PauseSource(m_audioSourceId);
}

bool MediaSession::Thread_StopCommand()
{
    if (m_mediaState == MediaState::Stopped)
    {
        return true;
    }

    m_mediaState = MediaState::Stopped;

    // Stop the media source
    m_mediaSource->Stop();

    // Flush the enqueued audio data
    m_audioManager->FlushEnqueuedData(m_audioSourceId);

    // Invalidate clocks
    m_externalClock.Invalidate();
    m_videoClock.Invalidate();
    m_audioClock.Invalidate();

    // Reset any accumulated audio sync data
    Thread_ResetAudioSyncDiff();

    // Reset next present points
    m_nextVideoPresentPoint = std::nullopt;
    m_nextAudioPresentPoint = std::nullopt;

    // Display the initial image for the session's texture
    m_renderer->UpdateTexture(m_textureId, m_initialImage);

    return true;
}

bool MediaSession::Thread_SeekByOffsetCommand(MediaDuration mediaDuration)
{
    // Only allowed to seek if we're in playing or paused state
    if (m_mediaState != MediaState::Playing && m_mediaState != MediaState::Paused)
    {
        return false;
    }

    const auto masterClockTime = GetMasterClockMediaPoint(std::chrono::steady_clock::now());
    if (!masterClockTime)
    {
        return false;
    }

    return Thread_SeekToPointCommand(*masterClockTime + mediaDuration);
}

bool MediaSession::Thread_SeekToPointCommand(MediaPoint mediaPoint)
{
    // Only allowed to seek if we're in playing or paused state
    if (m_mediaState != MediaState::Playing && m_mediaState != MediaState::Paused)
    {
        return false;
    }

    // Calculate how far from the current clock point we're seeking. Used by the container to
    // constrain the allowed seek range.
    auto seekStartMediaPoint = GetMasterClockMediaPoint(std::chrono::steady_clock::now());
    if (!seekStartMediaPoint)
    {
        seekStartMediaPoint = MediaPoint(0.0);
    }

    const auto seekOffset = MediaDuration(mediaPoint - *seekStartMediaPoint);

    // Stop and flush out all audio that was previously enqueued for playback
    m_audioManager->FlushEnqueuedData(m_audioSourceId);

    // As we're seeking by an unknown amount (seeking might snap to keyframes), invalidate our clocks; they'll
    // be restarted once we start presenting new frames from the new source location
    m_videoClock.Invalidate();
    m_audioClock.Invalidate();
    m_externalClock.Invalidate();

    // Reset any accumulated audio sync data
    Thread_ResetAudioSyncDiff();

    // Reset next present points
    m_nextVideoPresentPoint = std::nullopt;
    m_nextAudioPresentPoint = std::nullopt;

    if (m_mediaState == MediaState::Playing)
    {
        m_seekSourceState = MediaState::Playing;
    }
    else if (m_mediaState == MediaState::Paused)
    {
        m_seekSourceState = MediaState::Paused;
    }

    m_mediaState = MediaState::Seeking;

    // Re-target the media source to load data from (near to) the specified point
    return m_mediaSource->LoadFromPoint(mediaPoint, seekOffset);
}

bool MediaSession::Thread_LoadStreamsCommand(const std::unordered_set<unsigned int>& streamIndices)
{
    // Fetch the current master clock point
    auto curPoint = GetMasterClockMediaPoint(std::chrono::steady_clock::now());

    // If we're stopped, and thus don't have a master clock point, for the sake of letting streams load
    // successfully we'll just use the 0.0 point, which will cause the container to seek itself to the
    // beginning after the streams are loaded
    if (m_mediaState == MediaState::Stopped)
    {
        curPoint = MediaPoint(0.0);
    }

    // If we otherwise don't know what media point we're at, we can't change streams, as changing
    // streams requires a re-seek to the current media point to synchronize the streams
    if (!curPoint)
    {
        return false;
    }

    // Invalidate clocks
    m_videoClock.Invalidate();
    m_audioClock.Invalidate();
    m_externalClock.Invalidate();

    // Stop and flush out all audio that was previously enqueued for playback
    m_audioManager->FlushEnqueuedData(m_audioSourceId);

    // Reset any accumulated audio sync data
    Thread_ResetAudioSyncDiff();

    // Reset next present points
    m_nextVideoPresentPoint = std::nullopt;
    m_nextAudioPresentPoint = std::nullopt;

    // Tell the media source to load the streams
    return m_mediaSource->LoadStreams(*curPoint, streamIndices);
}

bool MediaSession::Thread_PresentVideoFrame(const std::chrono::steady_clock::time_point& now)
{
    const bool videoIsMasterClock = m_masterClockType == MasterClockType::Video;
    const bool externalIsMasterClock = m_masterClockType == MasterClockType::External;

    auto masterClockPoint = GetMasterClockMediaPoint(now);

    //
    // If either video or the external clock is the master clock, and the master clock is invalid, then
    // we need to forcefully display a frame of video below so that the clock can get a time set from it
    //
    const bool initializingVideoClock = videoIsMasterClock && !masterClockPoint;
    const bool initializingExternalClock = externalIsMasterClock && !masterClockPoint;

    //
    // If we're not forcefully presenting in order to initialize a clock, and if the master clock
    // doesn't have a time point, then just bail out and try again later when the master clock knows
    // where we're at.
    //
    if (!initializingVideoClock && !initializingExternalClock && !masterClockPoint)
    {
        m_nextVideoPresentPoint = std::nullopt;
        return false;
    }

    //
    // Get the current queue size at the time that this method is called.
    //
    // Note that the decoder thread will be re-filling the queue in parallel as we pop from it, so we
    // explicitly only operate on the items in the queue at the start of execution, to avoid continuously
    // processing new frames that are being enqueued in parallel
    //
    auto videoFrameQueueSize = m_mediaSource->GetVideoFrameQueueSize();

    // If there's nothing in the queue bail out as there's no frames to try to present or drop
    if (videoFrameQueueSize == 0)
    {
        LogDebug("MediaSession::Thread_PresentVideoFrame: Video queue ran dry");
        m_nextVideoPresentPoint = std::nullopt;
        return false;
    }

    //
    // If we're initializing the video or external clock, or if we have a valid video present point, then
    // we want to present the frame on the top of the video queue now
    //
    bool presentedVideoFrame = false;

    if (initializingVideoClock || initializingExternalClock || m_nextVideoPresentPoint)
    {
        const auto videoFrame = m_mediaSource->PopFrontVideoFrame();
        videoFrameQueueSize--;

        m_renderer->UpdateTexture(m_textureId, videoFrame->imageData);

        presentedVideoFrame = true;

        // When we display a video frame we update the video clock to sync it to that new time point
        Thread_UpdateVideoClock(videoFrame->presentPoint, now);

        // Re-fetch the master clock time now that we've presented a frame, as it might have initialized the master clock
        masterClockPoint = GetMasterClockMediaPoint(now);
    }

    unsigned int numDroppedFrames = 0;

    //
    // Flush through remaining items in the video queue, dropping frames off the top that are past
    // due to be presented, until we have either a future-dated frame on top, or nothing left in the
    // queue.
    //
    for (std::size_t x = 0; x < videoFrameQueueSize; ++x)
    {
        const auto nextVideoFrame = m_mediaSource->PeekFrontVideoFrame();
        if (!nextVideoFrame)
        {
            m_nextVideoPresentPoint = std::nullopt;
            return presentedVideoFrame;
        }

        if (nextVideoFrame->presentPoint <= *masterClockPoint)
        {
            (void)m_mediaSource->PopFrontVideoFrame();
            numDroppedFrames++;
            continue;
        }
    }

    if (numDroppedFrames > 0)
    {
        LogWarning("MediaSession: Dropped {} video frames", numDroppedFrames);
    }

    const auto nextVideoFrame = m_mediaSource->PeekFrontVideoFrame();
    if (nextVideoFrame)
    {
        m_nextVideoPresentPoint = nextVideoFrame->presentPoint;
        return presentedVideoFrame;
    }

    // Nothing left in the queue
    m_nextVideoPresentPoint = std::nullopt;

    return presentedVideoFrame;
}

void MediaSession::Thread_PresentAudioFrame(const std::chrono::steady_clock::time_point& now)
{
    const bool audioIsMasterClock = m_masterClockType == MasterClockType::Audio;
    const bool externalIsMasterClock = m_masterClockType == MasterClockType::External;

    auto masterClockPoint = GetMasterClockMediaPoint(now);

    //
    // If either audio or the external clock is the master clock, and the master clock is invalid, then
    // we need to forcefully enqueue a frame of audio below so that the clock can get a time set from it
    //
    const bool initializingAudioClock = audioIsMasterClock && !masterClockPoint;
    const bool initializingExternalClock = externalIsMasterClock && !masterClockPoint;

    //
    // If we're not forcefully presenting in order to initialize a clock, and if the master clock
    // doesn't have a time point, then just bail out and try again later when the master clock knows
    // where we're at.
    //
    if (!initializingAudioClock && !initializingExternalClock && !masterClockPoint)
    {
        m_nextAudioPresentPoint = std::nullopt;
        return;
    }

    //
    // Get the current queue size at the time that this method is called.
    //
    // Note that the decoder thread will be re-filling the queue in parallel as we pop from it, so we
    // explicitly only operate on the items in the queue at the start of execution, to avoid continuously
    // processing new frames that are being enqueued in parallel.
    //
    auto audioFrameQueueSize = m_mediaSource->GetAudioFrameQueueSize();

    // If there's nothing in the queue bail out as there's no frames to try to present or drop
    if (audioFrameQueueSize == 0)
    {
        LogDebug("MediaSession::Thread_PresentAudioFrame: Audio queue ran dry");
        m_nextAudioPresentPoint = std::nullopt;
        return;
    }

    //
    // If we're initializing a clock, then forcefully enqueue the first audio frame in the
    // queue, in order to initialize the clock with a time
    //
    if (initializingAudioClock || initializingExternalClock)
    {
        const auto audioFrame = m_mediaSource->PopFrontAudioFrame();
        audioFrameQueueSize--;

        (void)m_audioManager->EnqueueStreamedData(
            m_audioSourceId,
            {audioFrame->audioData},
            audioFrame->presentPoint.count(),
            true
        );

        // Update the audio clock from the audio data that was just enqueued
        Thread_UpdateAudioClock(now);

        // Re-fetch the master clock time now that we've presented a frame, which might have initialized the master clock
        masterClockPoint = GetMasterClockMediaPoint(now);
    }

    //
    // If audio isn't the master clock, and we don't yet have a next present point, flush the audio queue of
    // any useless past-timed audio frames in order to quickly bring the queue up towards the current clock
    // time, before starting to enqueue any audio data for playback. (Mostly useful when seeking, as the first
    // audio frames popped after seeking can be relatively far from the first video frame, in presentation time).
    //
    unsigned int numDroppedFrames = 0;

    if (!audioIsMasterClock && !m_nextAudioPresentPoint)
    {
        while (audioFrameQueueSize > 0)
        {
            const auto audioFrame = m_mediaSource->PeekFrontAudioFrame();

            // If the frame's play time is in the past, pop it off and drop it, and loop again
            if ((audioFrame->presentPoint + audioFrame->audioData->Duration()) < *masterClockPoint)
            {
                (void)m_mediaSource->PopFrontAudioFrame();
                audioFrameQueueSize--;
                numDroppedFrames++;
                continue;
            }

            // Otherwise, we found a valid next audio frame, keep it in the queue and break out
            break;
        }
    }

    if (numDroppedFrames != 0)
    {
        LogWarning("MediaSession: Dropped {} audio frames to fast-sync stream", numDroppedFrames);
    }

    //
    // Loop through the remaining items in the queue, looking for audio data that we can enqueue now
    //
    std::deque<AudioFrame> poppedAudioFrames;
    std::optional<MediaPoint> nextPresentPoint;

    for (unsigned int x = 0; x < audioFrameQueueSize; ++x)
    {
        auto audioFrame = m_mediaSource->PeekFrontAudioFrame();

        // If the frame is further than a second in the future, stopping looking, just record the frame's
        // present point as the next audio present point and break out
        if (masterClockPoint && (audioFrame->presentPoint > (*masterClockPoint + std::chrono::duration<double>(0.5))))
        {
            nextPresentPoint = audioFrame->presentPoint;
            break;
        }

        // Otherwise, pop the frame off to be enqueued for playback and loop again
        poppedAudioFrames.push_back(*audioFrame);
        (void)m_mediaSource->PopFrontAudioFrame();
    }

    // Transform AudioFrames to AudioDatas
    std::vector<Common::AudioData::Ptr> poppedAudioData;

    std::ranges::transform(poppedAudioFrames, std::back_inserter(poppedAudioData), [](const AudioFrame& audioFrame){
        return audioFrame.audioData;
    });

    // Enqueue all the popped audio datas for playback
    if (!poppedAudioData.empty())
    {
        if (!m_audioManager->EnqueueStreamedData(
            m_audioSourceId,
            poppedAudioData,
            poppedAudioFrames.front().presentPoint.count(),
            true
        ))
        {
            LogError("MediaSession::Thread_PresentAudioFrame: Failed to enqueue audio data for playback");
        }

        // Update audio clock after enqueuing more audio data
        Thread_UpdateAudioClock(now);
    }

    m_nextAudioPresentPoint = nextPresentPoint;
}

void MediaSession::Thread_RecordAudioSyncDiff(const std::chrono::steady_clock::time_point& now)
{
    // If the master clock is audio, we don't need to sync audio at all
    if (m_masterClockType == MasterClockType::Audio)
    {
        return;
    }

    // Get the latest clock timings
    const auto masterClockPoint = GetMasterClockMediaPoint(now);
    const auto audioClockPoint = m_audioClock.InterpolatedTime(now);

    if (!masterClockPoint || !audioClockPoint)
    {
        return;
    }

    // The timing diff between where the audio clock is and where the master clock is
    const auto audioOffset = *audioClockPoint - *masterClockPoint;

    // Accumulate the diff as another data point / sample
    m_audioDiffCum += audioOffset;
    m_audioDiffSamples++;

    // If we don't have enough data points, nothing else to do
    if (m_audioDiffSamples < AUDIO_SYNC_SAMPLE_SIZE)
    {
        return;
    }

    // Otherwise, we can calculate the average audio sync diff over the sample period
    const auto avgDiff = m_audioDiffCum / m_audioDiffSamples;

    // Tell the media source the current audio sync diff so that it can adjust for it
    // by activating sampling compensation when decoding audio packets
    m_mediaSource->SetAudioSyncDiff(avgDiff);

    // Clear out the accumulated audio diff so that it can be rebuilt again
    m_audioDiffCum = MediaDuration(0.0);
    m_audioDiffSamples = 0;

    m_metrics->SetDoubleValue("AUDIO_SYNC_DIFF", avgDiff.count());
}

void MediaSession::Thread_ResetAudioSyncDiff()
{
    m_audioDiffSamples = 0;
    m_audioDiffCum = MediaDuration(0.0);
    m_mediaSource->SetAudioSyncDiff(m_audioDiffCum);
}

void MediaSession::Thread_UpdateVideoClock(const MediaPoint& syncPoint, const std::chrono::steady_clock::time_point& now)
{
    // Update the video clock from the sync position/time
    m_videoClock.SetExplicit(syncPoint, now);

    // If the external clock hasn't been started, start it ticking from the video clock's time
    if (!m_externalClock.IsValid())
    {
        Thread_UpdateExternalClock(*m_videoClock.syncPoint, now);
    }
}

void MediaSession::Thread_UpdateAudioClock(const std::chrono::steady_clock::time_point& now)
{
    const auto audioSourceState = m_audioManager->GetSourceState(m_audioSourceId);
    if (!audioSourceState || !audioSourceState->playTime)
    {
        m_audioClock.Invalidate();
        return;
    }

    // Account for the OpenAL source elapsed time query not being continuous; it updates at an interval (~20ms).
    // We want to only sync to the reported audio time if it has actually moved forwards by a non-rounding error
    // amount of time.
    if (m_audioClock.IsValid())
    {
        const auto clockDiff = std::abs(m_audioClock.syncPoint->count() - *audioSourceState->playTime);
        if (clockDiff < 0.001)
        {
            return;
        }
    }

    // Update the audio clock from the audio source's play time
    m_audioClock.SetExplicit(MediaPoint(*audioSourceState->playTime), now);

    // If the external clock hasn't been started, start it ticking from the audio clock's time
    if (!m_externalClock.IsValid())
    {
        Thread_UpdateExternalClock(*m_audioClock.syncPoint, now);
    }
}

void MediaSession::Thread_UpdateExternalClock(const MediaPoint& syncPoint, const std::chrono::steady_clock::time_point& now)
{
    m_externalClock.SetExplicit(syncPoint, now);
}

const Clock& MediaSession::GetMasterClock() const
{
    switch (m_masterClockType)
    {
        case MasterClockType::External: return m_externalClock;
        case MasterClockType::Video: return m_videoClock;
        case MasterClockType::Audio: return m_audioClock;
    }

    assert(false);
    return m_externalClock;
}

std::optional<MediaPoint> MediaSession::GetMasterClockMediaPoint(const std::chrono::steady_clock::time_point& now) const
{
    const auto& masterClock = GetMasterClock();

    switch (m_mediaState)
    {
        // If we're in playing state, return the interpolated clock time, which is the clock's last sync point plus
        // the amount of time that has elapsed between when the clock was last synced and the current time
        case MediaState::Playing:
        {
            return masterClock.InterpolatedTime(now);
        }

        // Otherwise, in any other state our clock time shouldn't be increasing; just return the clock's last sync
        // point, ignoring how much time has elapsed in the real world since that point
        case MediaState::Paused:
        case MediaState::Seeking:
        case MediaState::Stopped:
        {
            return masterClock.syncPoint;
        }
    }

    assert(false);
    return std::nullopt;
}

}
