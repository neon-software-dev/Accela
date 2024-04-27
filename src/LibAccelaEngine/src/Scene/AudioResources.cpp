/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioResources.h"

#include "../Audio/AudioManager.h"

namespace Accela::Engine
{

AudioResources::AudioResources(Common::ILogger::Ptr logger, AudioManagerPtr audioManager)
    : m_logger(std::move(logger))
    , m_audioManager(std::move(audioManager))
{

}

bool AudioResources::RegisterAudio(const std::string& name, const Common::AudioData::Ptr& audioData)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading audio: {}", name);

    if (m_audio.contains(name))
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::RegisterAudio: Audio with name has already been registered: {}", name);
        return false;
    }

    if (!m_audioManager->RegisterAudio(name, audioData))
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::RegisterAudio: Failed to register audio with the audio manager: {}", name);
        return false;
    }

    m_audio.insert(name);

    return true;
}

void AudioResources::DestroyAudio(const std::string& name)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources::DestroyAudio: Destroying audio: {}", name);

    if (!m_audio.contains(name))
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::DestroyAudio: No such audio has been registered: {}", name);
        return;
    }

    m_audioManager->DestroyAudio(name);

    m_audio.erase(name);
}

void AudioResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources::DestroyAll: Destroying all audio");

    while (!m_audio.empty())
    {
        DestroyAudio(*m_audio.cbegin());
    }
}

}
