/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_COMPONENT_AUDIOCOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_AUDIOCOMPONENT_H

#include "Accela/Engine/Audio/AudioCommon.h"

#include <unordered_map>

namespace Accela::Engine
{
    enum class PlaybackState
    {
        NotStarted,
        Started,
        Stopped
    };

    struct AudioState
    {
        PlaybackState playbackState{PlaybackState::NotStarted};
    };

    struct AudioComponent
    {
        std::unordered_map<AudioSourceId, AudioState> activeSounds;
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_AUDIOCOMPONENT_H
