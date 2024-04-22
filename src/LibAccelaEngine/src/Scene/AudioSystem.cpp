/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioSystem.h"

#include "../Audio/AudioManager.h"

#include <optional>
#include <unordered_set>

namespace Accela::Engine
{

AudioSystem::AudioSystem(Common::ILogger::Ptr logger, AudioManagerPtr audioManager)
    : m_logger(std::move(logger))
    , m_audioManager(std::move(audioManager))
{

}

void AudioSystem::Execute(const RunState::Ptr&, entt::registry& registry)
{
    //
    // Update the position/properties of the audio "listener". This usually
    // corresponds in some way to the camera position.
    //
    UpdateAudioListener();

    //
    // Update the audio properties of any entity with both an audio component and a
    // transform component, so the audio source is attached to the entities' position
    // in the world.
    //
    registry.view<AudioComponent, TransformComponent>().each(
        [&](const entt::entity&, AudioComponent& audioComponent, const TransformComponent& transformComponent)
        {
            UpdateSourceProperties(audioComponent, transformComponent);
        }
    );

    //
    // For all entities with an audio component, start and stop their audio as needed
    //
    registry.view<AudioComponent>().each(
        [&](const entt::entity& entity, AudioComponent& audioComponent)
        {
            StartAndStopAudio(registry, (EntityId)entity, audioComponent);
        }
    );


    //
    // Automatically destroy any global sounds that have finished playing, without needing
    // the user to explicit stop them
    //
    m_audioManager->FulfillFinishedGlobalSources();
}

void AudioSystem::UpdateAudioListener()
{
    m_audioManager->UpdateListenerProperties(m_listener);
}

void AudioSystem::UpdateSourceProperties(AudioComponent& audioComponent, const TransformComponent& transformComponent) const
{
    for (auto& activeSoundIt : audioComponent.activeSounds)
    {
        m_audioManager->UpdateSourceProperties(activeSoundIt.first, transformComponent.GetPosition());
    }
}

void AudioSystem::StartAndStopAudio(entt::registry& registry, const EntityId& entity, AudioComponent& audioComponent)
{
    std::unordered_set<AudioSourceId> destroyedSources;

    //
    // Process every active sound associated with the entity
    //
    for (auto& activeSoundIt : audioComponent.activeSounds)
    {
        // If the sound hasn't been started yet, start it
        if (activeSoundIt.second.playbackState == PlaybackState::NotStarted)
        {
            m_logger->Log(Common::LogLevel::Debug,
              "StartAndStopAudio: Starting sound, source id: {}", activeSoundIt.first);
            m_audioManager->PlaySource(activeSoundIt.first);
            activeSoundIt.second.playbackState = PlaybackState::Started;
        }
        // Otherwise, if the sound has finished playing, destroy it
        else if (m_audioManager->IsSourceStopped(activeSoundIt.first))
        {
            m_logger->Log(Common::LogLevel::Debug,
             "StartAndStopAudio: Sound finished playing, destroying it, source id: {}", activeSoundIt.first);
            m_audioManager->DestroySource(activeSoundIt.first);
            destroyedSources.insert(activeSoundIt.first);
        }
        // Otherwise, if the user requested the sound to be stopped, stop it
        else if (activeSoundIt.second.playbackState == PlaybackState::Stopped)
        {
            m_logger->Log(Common::LogLevel::Debug,
              "StartAndStopAudio: Sound was stopped, destroying it, source id: {}", activeSoundIt.first);
            m_audioManager->StopSource(activeSoundIt.first);
            m_audioManager->DestroySource(activeSoundIt.first);
            destroyedSources.insert(activeSoundIt.first);
        }
    }

    //
    // Remove destroyed audio sources from the entity's audio component
    //
    for (const auto& destroyedSource : destroyedSources)
    {
        audioComponent.activeSounds.erase(destroyedSource);
    }

    //
    // If the audio component is no longer tracking any audio, destroy it
    //
    if (audioComponent.activeSounds.empty())
    {
        registry.erase<AudioComponent>((entt::entity)entity);
    }
}

void AudioSystem::SetAudioListener(const AudioListener& listener)
{
    m_listener = listener;
}

}
