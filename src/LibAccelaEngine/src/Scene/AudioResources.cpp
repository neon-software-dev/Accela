/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioResources.h"

#include "../Util.h"

#include "../Audio/AudioManager.h"

#include <Accela/Engine/IEngineAssets.h>

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

struct BoolResultMessage : public Common::ResultMessage<bool>
{
    BoolResultMessage()
        : Common::ResultMessage<bool>("BoolResultMessage")
    { }
};

AudioResources::AudioResources(Common::ILogger::Ptr logger,
                               std::shared_ptr<IEngineAssets> assets,
                               std::shared_ptr<Platform::IFiles> files,
                               AudioManagerPtr audioManager,
                               std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_assets(std::move(assets))
    , m_files(std::move(files))
    , m_audioManager(std::move(audioManager))
    , m_threadPool(std::move(threadPool))
{

}

std::future<bool> AudioResources::LoadAssetsAudio(const std::string& audioFileName)
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<BoolResultMessage>(message)->SetResult(
            OnLoadAssetsAudio(audioFileName)
        );
    });

    return messageFuture;
}

std::future<bool> AudioResources::LoadAllAssetAudio()
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<BoolResultMessage>(message)->SetResult(
            OnLoadAllAssetAudio()
        );
    });

    return messageFuture;
}

bool AudioResources::OnLoadAssetsAudio(const std::string& audioFileName)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading asset audio: {}", audioFileName);

    const auto splitFileName = SplitFileName(audioFileName);
    if (!splitFileName)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::OnLoadAssetsAudio: Invalid audio file name: {}", audioFileName);
        return false;
    }

    const auto audioExpect = m_assets->ReadAudioBlocking(audioFileName);
    if (!audioExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::OnLoadAssetsAudio: Failed to read asset audio: {}", audioFileName);
        return false;
    }

    return LoadAudio(splitFileName->first, *audioExpect);
}

bool AudioResources::OnLoadAllAssetAudio()
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading all asset audio");

    const auto allAudioFiles = m_files->ListFilesInAssetsSubdir(Platform::AUDIO_SUBDIR);
    if (!allAudioFiles)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::OnLoadAllAssetAudio: Failed to list files in audio directory");
        return false;
    }

    bool allSuccessful = true;

    for (const auto& audioFileName : *allAudioFiles)
    {
        allSuccessful = allSuccessful && OnLoadAssetsAudio(audioFileName);
    }

    return allSuccessful;
}

bool AudioResources::LoadAudio(const std::string& name, const Common::AudioData::Ptr& audioData)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading audio: {}", name);

    std::lock_guard<std::mutex> audioLock(m_audioMutex);

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
