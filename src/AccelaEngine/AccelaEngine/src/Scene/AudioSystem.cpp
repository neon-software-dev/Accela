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
    // For all entities with an audio component, destroy any static audio sources which have finished
    // playing. (However, for streamed sources, we keep those around, even if they're temporarily "finished").
    //
    registry.view<AudioComponent>().each(
        [&](const entt::entity& entity, AudioComponent& audioComponent)
        {
            ProcessFinishedAudio(registry, (EntityId)entity, audioComponent);
        }
    );

    //
    // Clean up any finished transient audio sources
    //
    m_audioManager->DestroyFinishedTransientSources();

    //
    // Clean up played buffers for streamed audio sources
    //
    m_audioManager->DestroyFinishedStreamedData();
}

void AudioSystem::UpdateAudioListener()
{
    m_audioManager->UpdateAudioListener(m_listener);
}

void AudioSystem::UpdateSourceProperties(AudioComponent& audioComponent, const TransformComponent& transformComponent) const
{
    for (auto& activeSound : audioComponent.activeSounds)
    {
        (void)m_audioManager->UpdateLocalSourcePosition(activeSound, transformComponent.GetPosition());
    }
}

void AudioSystem::ProcessFinishedAudio(entt::registry& registry, const EntityId& entity, AudioComponent& audioComponent)
{
    std::unordered_set<AudioSourceId> finishedSources;

    //
    // Look for any static (non-streamed) audio sources associated with the entity
    //
    for (auto& sourceId : audioComponent.activeSounds)
    {
        const auto sourceDataType = m_audioManager->GetSourceDataType(sourceId);

        if (!sourceDataType || *sourceDataType != SourceDataType::Static)
        {
            continue;
        }

        const auto sourceState = m_audioManager->GetSourceState(sourceId);
        if (!sourceState || (sourceState->playState == PlayState::Stopped))
        {
            finishedSources.insert(sourceId);
            continue;
        }
    }

    //
    // Remove the finished audio sources from the entity's audio component
    //
    for (const auto& finishedSource : finishedSources)
    {
        LogInfo("AudioSystem: Detected finished audio {} for entity {}", finishedSource, entity);
        audioComponent.activeSounds.erase(finishedSource);
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
