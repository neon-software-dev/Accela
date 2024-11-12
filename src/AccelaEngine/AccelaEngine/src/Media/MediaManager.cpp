/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MediaManager.h"

#include <Accela/Engine/Scene/IWorldResources.h>

#include "Media/FFMPEG/FFMPEGContainer.h"
#include "Media/FFMPEG/FFMPEGMediaSource.h"

#include "../Audio/AudioManager.h"

#include "../Scene/PackageResources.h"

#include <Accela/Common/Thread/ThreadUtil.h>

#include <algorithm>

namespace Accela::Engine
{

MediaManager::MediaManager(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           IWorldResourcesPtr worldResources,
                           AudioManagerPtr audioManager,
                           Render::IRenderer::Ptr renderer)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_worldResources(std::move(worldResources))
    , m_audioManager(std::move(audioManager))
    , m_renderer(std::move(renderer))
{

}

bool MediaManager::Startup()
{
    return true;
}

void MediaManager::Shutdown()
{
    DestroyAll();
}

std::expected<MediaSessionId, bool> MediaManager::CreateURLMediaSession(const std::string& url,
                                                                        const AudioSourceProperties& audioSourceProperties,
                                                                        bool localAudioSource)
{
    LogInfo("MediaManager: Playing url media: {}", url);

    return CreateFFMPEGURLSession(url, audioSourceProperties, localAudioSource);
}

bool MediaManager::DoesMediaSessionExist(const MediaSessionId& mediaSessionId) const
{
    return m_sessions.contains(mediaSessionId);
}

std::optional<Render::TextureId> MediaManager::GetMediaSessionTextureId(const MediaSessionId& mediaSessionId) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        return std::nullopt;
    }

    return it->second->GetTextureId();
}

std::optional<AudioSourceId> MediaManager::GetMediaSessionAudioSourceId(const MediaSessionId& mediaSessionId) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        return std::nullopt;
    }

    return it->second->GetAudioSourceId();
}

std::expected<MediaSessionId, bool> MediaManager::CreateFFMPEGURLSession(const std::string& url,
                                                                         const AudioSourceProperties& audioSourceProperties,
                                                                         bool localAudioSource)
{
    LogInfo("MediaManager: Playing URL video: {}", url);

    //
    // Load/open the URL as an FFMPEG Media Source
    //
    FFMPEGContainer::Config ffmpegConfig{};
    ffmpegConfig.audioOutputFormat = localAudioSource ? Common::AudioDataFormat::Mono16 : Common::AudioDataFormat::Stereo16;

    auto ffmpegContainer = std::make_unique<FFMPEGContainer>(m_logger, ffmpegConfig);
    if (!ffmpegContainer->Open(url))
    {
        LogError("MediaManager::PlayURL: Failed to open FFMPEG URL: {}", url);
        return std::unexpected(false);
    }

    if (!ffmpegContainer->LoadBestStreams())
    {
        LogError("MediaManager::PlayURL: Failed to load best streams");
        ffmpegContainer->Destroy();
        return std::unexpected(false);
    }

    //
    // Create an initial/temporary image to be displayed in the texture until we start playing the media
    //
    const auto videoDimensions = ffmpegContainer->GetVideoStreamDimensions();
    if (!videoDimensions)
    {
        LogError("MediaManager::PlayURL: Container couldn't determine video dimensions");
        ffmpegContainer->Destroy();
        return std::unexpected(false);
    }

    // TODO! Why doesnt setting this to 128 create a grey looking image?
    const auto initialImageColor = std::byte(128);
    std::vector<std::byte> initialImageBytes(videoDimensions->first * videoDimensions->second * 4, initialImageColor);
    for (std::size_t x = 0; x < videoDimensions->first * videoDimensions->second; ++x)
    {
        initialImageBytes[(x * 4) + 3] = std::byte(255);
    }

    const auto initialDisplayImage = std::make_shared<Common::ImageData>(
        initialImageBytes,
        1,
        videoDimensions->first,
        videoDimensions->second,
        Common::ImageData::PixelFormat::RGBA32
    );

    //
    // Create an audio source for the media to play audio using
    //
    AudioSourceId audioSourceId = 0;

    if (localAudioSource)
    {
        const auto ret = m_audioManager->CreateLocalStreamedSource(audioSourceProperties, {0,0,0});
        if (!ret)
        {
            LogError("MediaManager::PlayURL: Failed to create local streamed audio source");
            ffmpegContainer->Destroy();
            return std::unexpected(false);
        }

        audioSourceId = *ret;
    }
    else
    {
        const auto ret = m_audioManager->CreateGlobalStreamedSource(audioSourceProperties);
        if (!ret)
        {
            LogError("MediaManager::PlayURL: Failed to create global streamed audio source");
            ffmpegContainer->Destroy();
            return std::unexpected(false);
        }

        audioSourceId = *ret;
    }

    //
    // Create a texture for the media to render video frames into
    //
    std::string textureTag;
    if (url.length() <= 30) { textureTag = url; }
    else {textureTag = url.substr(url.length() - 30); }

    const auto mediaTextureId = m_worldResources->Textures()->LoadCustomTexture(
        initialDisplayImage,
        TextureLoadConfig{
            .numMipLevels = 4, // TODO Perf: Tweak
            .uvAddressMode = std::nullopt
        },
        textureTag,
        Engine::ResultWhen::Ready
    ).get();

    //
    // Record state
    //
    const auto mediaSessionId = m_ids.GetId();

    auto mediaSource = std::make_shared<FFMPEGMediaSource>(m_logger, m_metrics, std::move(ffmpegContainer));

    auto mediaSession = std::make_unique<MediaSession>(
        m_logger,
        m_metrics,
        m_renderer,
        m_audioManager,
        mediaSessionId,
        std::move(mediaSource),
        initialDisplayImage,
        mediaTextureId,
        audioSourceId
    );

    m_sessions.insert({mediaSessionId, std::move(mediaSession)});

    return mediaSessionId;
}

std::future<bool> MediaManager::PlayMediaSession(const MediaSessionId& mediaSessionId, const std::optional<MediaPoint>& playPoint) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        LogError("MediaManager::PlayMediaSession: No such media session exists: {}", mediaSessionId.id);
        return Common::ImmediateFuture(false);
    }

    return it->second->Play(playPoint);
}

std::future<bool> MediaManager::PauseMediaSession(const MediaSessionId& mediaSessionId) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        LogError("MediaManager::PauseMediaSession: No such media session exists: {}", mediaSessionId.id);
        return Common::ImmediateFuture(false);
    }

    return it->second->Pause();
}

std::future<bool> MediaManager::StopMediaSession(const MediaSessionId& mediaSessionId) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        LogError("MediaManager::StopMediaSession: No such media session exists: {}", mediaSessionId.id);
        return Common::ImmediateFuture(false);
    }

    return it->second->Stop();
}

std::future<bool> MediaManager::SeekMediaSessionByOffset(const MediaSessionId& mediaSessionId, const MediaDuration& offset) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        LogError("MediaManager::SeekMediaSessionByOffset: No such media session exists: {}", mediaSessionId.id);
        return Common::ImmediateFuture(false);
    }

    return it->second->SeekByOffset(offset);
}

std::future<bool> MediaManager::LoadStreams(const MediaSessionId& mediaSessionId, const std::unordered_set<unsigned int>& streamIndices) const
{
    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        LogError("MediaManager::LoadStreams: No such media session exists: {}", mediaSessionId.id);
        return Common::ImmediateFuture(false);
    }

    return it->second->LoadStreams(streamIndices);
}

void MediaManager::DestroySession(const MediaSessionId& mediaSessionId)
{
    LogInfo("MediaManager: Destroying session: {}", mediaSessionId.id);

    const auto it = m_sessions.find(mediaSessionId);
    if (it == m_sessions.cend())
    {
        LogWarning("MediaManager::DestroySession: No such session exists: {}", mediaSessionId.id);
        return;
    }

    const auto sessionTextureId = it->second->GetTextureId();
    const auto audioSourceId = it->second->GetAudioSourceId();

    // Destroy the session
    it->second->Destroy();

    // Destroy the texture the session was rendering into
    m_worldResources->Textures()->DestroyTexture(sessionTextureId);

    // Destroy the audio source the session was using
    m_audioManager->DestroySource(audioSourceId);

    // Erase knowledge of the session
    m_sessions.erase(it);
}

void MediaManager::DestroyAll()
{
    LogInfo("MediaManager: Destroying all");

    while (!m_sessions.empty())
    {
        DestroySession(m_sessions.cbegin()->first);
    }

    m_ids.Reset();
}

}
