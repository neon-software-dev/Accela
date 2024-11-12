/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioResources.h"
#include "PackageResources.h"

#include "../Audio/AudioManager.h"
#include "../Audio/AudioUtil.h"

#include <Accela/Common/Thread/ResultMessage.h>
#include <Accela/Common/Thread/ThreadUtil.h>

#include <cstring>

namespace Accela::Engine
{

AudioResources::AudioResources(Common::ILogger::Ptr logger,
                               PackageResourcesPtr packages,
                               AudioManagerPtr audioManager,
                               std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_packages(std::move(packages))
    , m_audioManager(std::move(audioManager))
    , m_threadPool(std::move(threadPool))
{

}

std::future<bool> AudioResources::LoadAudio(const PackageResourceIdentifier& resource)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAudio(resource)
        );
    });

    return messageFuture;
}

bool AudioResources::OnLoadAudio(const PackageResourceIdentifier& resource)
{
    const auto package = m_packages->GetPackageSource(*resource.GetPackageName());
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::OnLoadAudio: No such package: {}", resource.GetPackageName()->name);
        return false;
    }

    return LoadPackageAudio(*package, resource);
}

bool AudioResources::LoadAudio(const CustomResourceIdentifier& resource, const Common::AudioData::Ptr& audioData)
{
    m_logger->Log(Common::LogLevel::Info,
      "AudioResources: Loading custom audio resource: {}", resource.GetUniqueName());

    if (m_customAudio.contains(resource))
    {
        m_logger->Log(Common::LogLevel::Warning,
          "AudioResources::LoadAudio: Custom audio resource already exists, ignoring: {}", resource.GetUniqueName());
        return true;
    }

    if (!m_audioManager->LoadResourceAudio(resource, audioData))
    {
        m_logger->Log(Common::LogLevel::Warning,
          "AudioResources::LoadAudio: Failed to register audio: {}", resource.GetUniqueName());
        return false;
    }

    m_customAudio.insert(resource);

    return true;
}

std::future<bool> AudioResources::LoadAllAudio(const PackageName& packageName)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAllAudio(packageName)
        );
    });

    return messageFuture;
}

bool AudioResources::OnLoadAllAudio(const PackageName& packageName)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading all audio resources for package: {}", packageName.name);

    const auto package = m_packages->GetPackageSource(packageName);
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::OnLoadAllAudio: No such package exists: {}", packageName.name);
        return false;
    }

    const auto audioResourceNames = (*package)->GetAudioResourceNames();

    bool allSuccessful = true;

    for (const auto& audioResourceName : audioResourceNames)
    {
        allSuccessful = allSuccessful && LoadPackageAudio(*package, PRI(packageName, audioResourceName));
    }

    return allSuccessful;
}

std::future<bool> AudioResources::LoadAllAudio()
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAllAudio()
        );
    });

    return messageFuture;
}

bool AudioResources::OnLoadAllAudio()
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading all audio for all packages");

    bool allSuccessful = true;

    for (const auto& package : m_packages->GetAllPackages())
    {
        allSuccessful = allSuccessful && OnLoadAllAudio(PackageName(package->GetPackageName()));
    }

    return allSuccessful;
}

void AudioResources::DestroyAudio(const ResourceIdentifier& resource)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Destroying audio resource: {}", resource.GetUniqueName());

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Destroy the audio in the audio manager
    m_audioManager->DestroyResourceAudio(resource);

    // Erase our knowledge of the resource
    if (resource.IsPackageResource())
    {
        const auto it = m_packageAudio.find(*resource.GetPackageName());
        if (it == m_packageAudio.cend())
        {
            m_logger->Log(Common::LogLevel::Error,
              "AudioResources::DestroyAudio: No package tracking entry for: {}", resource.GetUniqueName());
            return;
        }

        // Erase our knowledge of the audio resource within its package
        it->second.erase(resource);

        // If the package has no audio left, erase our record of it too
        if (it->second.empty())
        {
            m_packageAudio.erase(it);
        }
    }
    else
    {
        m_customAudio.erase(resource);
    }
}

void AudioResources::DestroyAllAudio(const PackageName& packageName)
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Destroying all audio resource for package: {}", packageName.name);

    std::lock_guard<std::mutex> lock(m_audioMutex);

    const auto it = m_packageAudio.find(packageName);
    if (it == m_packageAudio.cend())
    {
        return;
    }

    for (const auto& resource : it->second)
    {
        m_logger->Log(Common::LogLevel::Info, "AudioResources: Destroying audio resource: {}", resource.GetUniqueName());

        // Destroy the resource in the audio manager
        m_audioManager->DestroyResourceAudio(resource);
    }

    // Erase our knowledge of the package
    m_packageAudio.erase(it);
}

void AudioResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "AudioResources: Destroying all audio resources");

    while (!m_customAudio.empty())
    {
        DestroyAudio(*m_customAudio.cbegin());
    }

    while (!m_packageAudio.empty())
    {
        DestroyAllAudio(m_packageAudio.cbegin()->first);
    }
}

bool AudioResources::LoadPackageAudio(const Platform::PackageSource::Ptr& package, const PackageResourceIdentifier& resource)
{
    const auto packageName = PackageName(package->GetPackageName());

    m_logger->Log(Common::LogLevel::Info, "AudioResources: Loading package audio resource: {}", resource.GetUniqueName());

    std::lock_guard<std::mutex> lock(m_audioMutex);

    auto audioBytes = package->GetAudioData(resource.GetResourceName());
    if (!audioBytes)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::LoadPackageAudio: Failed to get audio bytes: {}", resource.GetUniqueName());
        return false;
    }

    const auto audioData = AudioDataFromBytes(*audioBytes, resource.GetResourceName());
    if (!audioData)
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::LoadPackageAudio: Failed to create audio data: {}", resource.GetUniqueName());
        return false;
    }

    if (!m_audioManager->LoadResourceAudio(resource, *audioData))
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::LoadPackageAudio: Failed to register audio: {}", resource.GetUniqueName());
        return false;
    }

    m_packageAudio[packageName].insert(resource);

    return true;
}

std::expected<Common::AudioData::Ptr, bool> AudioResources::AudioDataFromBytes(std::vector<std::byte>& bytes, const std::string& tag) const
{
    // TODO Perf: Can we avoid the copy?
    std::vector<uint8_t> audioUInts(bytes.size());
    memcpy(audioUInts.data(), bytes.data(), bytes.size());

    AudioFile<double> audioFile;
    if (!audioFile.loadFromMemory(audioUInts))
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::AudioDataFromBytes: Failed to load audio file from bytes: {}", tag);
        return std::unexpected(false);
    }

    Common::AudioDataFormat audioFileFormat{};

    if (audioFile.getNumChannels() == 1 && audioFile.getBitDepth() == 8)
    {
        audioFileFormat = Common::AudioDataFormat::Mono8;
    }
    else if (audioFile.getNumChannels() == 1)
    {
        // We transform all bit depth >= 16 to 16 bit as that's the most OpenAL supports
        audioFileFormat = Common::AudioDataFormat::Mono16;
    }
    else if (audioFile.getNumChannels() == 2 && audioFile.getBitDepth() == 8)
    {
        audioFileFormat = Common::AudioDataFormat::Stereo8;
    }
    else if (audioFile.getNumChannels() == 2)
    {
        // We transform all bit depth >= 16 to 16 bit as that's the most OpenAL supports
        audioFileFormat = Common::AudioDataFormat::Stereo16;
    }
    else
    {
        m_logger->Log(Common::LogLevel::Error,
          "AudioResources::AudioDataFromBytes: Unsupported audio file: {}. Num channels: {}, bit depth: {}",
          tag,
          audioFile.getNumChannels(),
          audioFile.getBitDepth()
        );
        return std::unexpected(false);
    }

    std::vector<std::byte> audioByteBuffer = AudioUtil::AudioFileToByteBuffer(audioFile);

    return std::make_shared<Common::AudioData>(
        audioFileFormat,
        audioFile.getSampleRate(),
        audioByteBuffer
    );
}

}
