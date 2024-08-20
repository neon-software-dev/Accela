/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioManager.h"

#include <vector>
#include <string>
#include <cstring>

namespace Accela::Engine
{

AudioManager::AudioManager(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

std::vector<std::string> ParseALCStringList(const ALCchar* pStringList)
{
    if (pStringList == nullptr) { return {}; }

    std::vector<std::string> strs;

    const ALCchar *pChar = pStringList;

    while (*pChar != '\0')
    {
        const auto strLength = strlen(pChar);
        strs.emplace_back(pChar, strLength);
        pChar += strLength;
    }

    return strs;
}

bool AudioManager::Startup()
{
    m_logger->Log(Common::LogLevel::Info, "AudioManager initializing");

    ALenum error{AL_NO_ERROR};

    //
    // Determine the output device to use
    //
    const ALCchar* pAllDevices{nullptr};
    const ALCchar* pDefaultDeviceName{nullptr};

    if (alcIsExtensionPresent(nullptr, "ALC_enumerate_all_EXT"))
    {
        pAllDevices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        pDefaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
    }
    else
    {
        pAllDevices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        pDefaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    }

    const std::string defaultDevice(pDefaultDeviceName);
    const std::vector<std::string> allDevices = ParseALCStringList(pAllDevices);

    std::vector<std::string> candidateDevices
    {
        defaultDevice
    };

    for (const auto& device : allDevices)
    {
        if (device != defaultDevice)
        {
            candidateDevices.push_back(device);
        }
    }

    m_logger->Log(Common::LogLevel::Info, "AudioManager: Discovered {} audio device(s)", candidateDevices.size());

    //
    // Open an audio device for output
    //
    for (const auto& device : candidateDevices)
    {
        m_logger->Log(Common::LogLevel::Info, "AudioManager: Attempting to open output device: {}", device);

        m_pDevice = alcOpenDevice(device.c_str());
        if (m_pDevice == nullptr)
        {
            m_logger->Log(Common::LogLevel::Error, "AudioManager: Failed to open audio device: {}", device);
            continue;
        }

        m_logger->Log(Common::LogLevel::Info, "AudioManager: Using output device: {}", device);
    }

    if (m_pDevice == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "AudioManager: Exhausted all audio devices, aborting");
        return false;
    }

    //
    // Create an audio context
    //
    alGetError();
    m_pContext = alcCreateContext(m_pDevice, nullptr);
    if (m_pContext == nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "AudioManager: Failed to create audio context, error code: {}", alGetError());
        Shutdown();
        return false;
    }

    //
    // Activate the audio context
    //
    alGetError();
    alcMakeContextCurrent(m_pContext);
    if ((error = alGetError()) != AL_NO_ERROR)
    {
        m_logger->Log(Common::LogLevel::Error, "AudioManager: Failed to activate audio context, error code: {}", error);
        Shutdown();
        return false;
    }

    return true;
}

void AudioManager::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "AudioManager shutting down");

    // Unload any sounds + sources currently loaded
    DestroyAllAudio();

    // Shutdown and destroy the audio context + device
    alcMakeContextCurrent(nullptr);

    if (m_pContext != nullptr)
    {
        alcDestroyContext(m_pContext);
        m_pContext = nullptr;
    }

    if (m_pDevice != nullptr)
    {
        alcCloseDevice(m_pDevice);
        m_pDevice = nullptr;
    }
}

bool AudioManager::RegisterAudio(const ResourceIdentifier& resource, const Common::AudioData::Ptr& audioData)
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    if (m_pDevice == nullptr || m_pContext == nullptr) { return false; }

    m_logger->Log(Common::LogLevel::Info, "RegisterAudio: Registering audio: {}", resource.GetUniqueName());

    if (m_buffers.find(resource) != m_buffers.cend())
    {
        m_logger->Log(Common::LogLevel::Warning, "RegisterAudio: Audio already loaded, ignoring: {}", resource.GetUniqueName());
        return true;
    }

    ALenum error{AL_NO_ERROR};

    ALuint bufferId{0};

    alGetError();
    alGenBuffers(1, &bufferId);
    if ((error = alGetError()) != AL_NO_ERROR)
    {
        m_logger->Log(Common::LogLevel::Debug,
          "RegisterAudio: Failed to generate a buffer for audio: {}, error code: {}", resource.GetUniqueName(), error);
        return false;
    }

    ALenum audioFormat{0};

    switch (audioData->format)
    {
        case Common::AudioData::Format::Mono8:
            audioFormat = AL_FORMAT_MONO8;
        break;
        case Common::AudioData::Format::Mono16:
            audioFormat = AL_FORMAT_MONO16;
        break;
        case Common::AudioData::Format::Stereo8:
            audioFormat = AL_FORMAT_STEREO8;
        break;
        case Common::AudioData::Format::Stereo16:
            audioFormat = AL_FORMAT_STEREO16;
        break;
    }

    alGetError();
    alBufferData(bufferId, audioFormat, audioData->data.data(), (ALsizei)audioData->data.size(), (ALsizei)audioData->sampleRate);
    if ((error = alGetError()) != AL_NO_ERROR)
    {
        m_logger->Log(Common::LogLevel::Debug,
          "RegisterAudio: Failed to buffer data for sound: {}, error code: {}", resource.GetUniqueName(), error);
        alDeleteBuffers(1, &bufferId);
        return false;
    }

    m_buffers.insert(std::make_pair(resource, BufferProperties(bufferId, audioFormat)));

    return true;
}

void AudioManager::DestroyAudio(const ResourceIdentifier& resource)
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    const auto it = m_buffers.find(resource);
    if (it == m_buffers.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "DestroyAudio: Destroying audio: {}", resource.GetUniqueName());

    // Delete all the sources that were using this buffer
    while (!it->second.sources.empty())
    {
        const auto sourceId = *it->second.sources.cbegin();
        m_logger->Log(Common::LogLevel::Debug, "DestroyAudio: Destroying dependent active source: {}", sourceId);
        DestroySource(sourceId);
    }

    // Delete the buffer
    alDeleteBuffers(1, &it->second.bufferId);

    m_buffers.erase(it);
}

void AudioManager::DestroyAllAudio()
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    // Note that sources are destroyed as part of DestroyAudio
    while (!m_buffers.empty())
    {
        DestroyAudio(m_buffers.cbegin()->first);
    }
}

std::expected<AudioSourceId, bool>
AudioManager::CreateSource(const ResourceIdentifier& resource, const AudioManager::SourceProperties& properties)
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    if (m_pDevice == nullptr || m_pContext == nullptr) { return false; }

    auto bufferIt = m_buffers.find(resource);
    if (bufferIt == m_buffers.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "CreateSource: Unable to create source as audio is not loaded: {}", resource.GetUniqueName());
        return std::unexpected(false);
    }

    // Local (non-global) audio sources must be mono audio format or else 3D spatialization won't work
    if (properties.localSource)
    {
        if (bufferIt->second.bufferFormat != AL_FORMAT_MONO8 && bufferIt->second.bufferFormat != AL_FORMAT_MONO16)
        {
            m_logger->Log(Common::LogLevel::Error,
              "CreateSource: non-mono audio format local source is disallowed: {}", resource.GetUniqueName());
            return std::unexpected(false);
        }
    }

    ALenum error{AL_NO_ERROR};

    ALuint sourceId{0};

    alGetError();
    alGenSources(1, &sourceId);
    if ((error = alGetError()) != AL_NO_ERROR)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateSource: Failed to generate source for: {}, error code: {}", resource.GetUniqueName(), error);
        return std::unexpected(false);
    }

    alSourcei(sourceId, AL_BUFFER, (ALint)bufferIt->second.bufferId);
    alSourcei(sourceId, AL_LOOPING, properties.audioProperties.looping);
    alSourcef(sourceId, AL_REFERENCE_DISTANCE, properties.audioProperties.referenceDistance);
    alSourcef(sourceId, AL_GAIN, properties.audioProperties.gain);

    // Make a record of the source
    m_sources.insert(std::make_pair(sourceId, properties));
    m_sourceToResource.insert(std::make_pair(sourceId, resource));

    // Update the buffer's record to show that the source is using it
    bufferIt->second.sources.insert(sourceId);

    return sourceId;
}

void AudioManager::DestroySource(const AudioSourceId& sourceId)
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return; }

    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Debug, "DestroySource: Destroying source: {}", sourceId);

    // Mark the buffer as not being used by this source
    const auto bufferNameIt = m_sourceToResource.find(sourceId);
    if (bufferNameIt != m_sourceToResource.cend())
    {
        std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

        auto bufferIt = m_buffers.find(bufferNameIt->second);
        if (bufferIt != m_buffers.cend())
        {
            bufferIt->second.sources.erase(sourceId);
        }
    }

    // Delete the source
    alDeleteSources(1, &sourceIt->first);
    m_sourceToResource.erase(sourceId);
    m_sources.erase(sourceIt);
}

bool AudioManager::PlaySource(const AudioSourceId& sourceId)
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return false; }

    const auto it = m_sources.find(sourceId);
    if (it == m_sources.cend())
    {
        return false;
    }

    m_logger->Log(Common::LogLevel::Debug, "PlaySource: Starting play of source: {}", sourceId);

    alSourcePlay(it->first);

    return true;
}

bool AudioManager::IsSourceStopped(const AudioSourceId& sourceId)
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return false; }

    ALint sourceState{0};
    alGetSourcei(sourceId, AL_SOURCE_STATE, &sourceState);
    return sourceState == AL_STOPPED;
}

bool AudioManager::StopSource(const AudioSourceId& sourceId)
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return false; }

    m_logger->Log(Common::LogLevel::Debug, "StopSource: Stopping play of source: {}", sourceId);

    alSourceStop(sourceId);

    return true;
}

bool AudioManager::UpdateSourceProperties(const AudioSourceId& sourceId, const glm::vec3& position)
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return false; }

    const auto it = m_sources.find(sourceId);
    if (it == m_sources.cend())
    {
        return false;
    }

    alSource3f(sourceId, AL_POSITION, position.x, position.y, position.z);

    return true;
}

void AudioManager::FulfillFinishedGlobalSources()
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return; }

    //
    // Finds any global audio sources that have finished and destroys them
    //
    std::unordered_set<unsigned int> sourceIdsToDestroy;

    for (const auto& sourceIt : m_sources)
    {
        // Ignore local sources
        if (sourceIt.second.localSource)
        {
            continue;
        }

        if (IsSourceStopped(sourceIt.first))
        {
            sourceIdsToDestroy.insert(sourceIt.first);
        }
    }

    for (const auto& sourceId : sourceIdsToDestroy)
    {
        m_logger->Log(Common::LogLevel::Debug,
          "FulfillFinishedGlobalSources: Cleaning up finished global source: {}", sourceId);
        DestroySource(sourceId);
    }
}

void AudioManager::UpdateListenerProperties(const AudioListener& listener) const
{
    if (m_pDevice == nullptr || m_pContext == nullptr) { return; }

    alListener3f(AL_POSITION, listener.worldPosition.x, listener.worldPosition.y, listener.worldPosition.z);

    float orientationVals[6];
    orientationVals[0] = listener.lookUnit.x;
    orientationVals[1] = listener.lookUnit.y;
    orientationVals[2] = listener.lookUnit.z;
    orientationVals[3] = listener.upUnit.x;
    orientationVals[4] = listener.upUnit.y;
    orientationVals[5] = listener.upUnit.z;
    alListenerfv(AL_ORIENTATION, orientationVals);
}

}
