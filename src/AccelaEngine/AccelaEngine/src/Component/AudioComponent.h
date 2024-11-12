/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_COMPONENT_AUDIOCOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_AUDIOCOMPONENT_H

#include "Accela/Engine/Audio/AudioCommon.h"

#include <unordered_set>

namespace Accela::Engine
{
    struct AudioComponent
    {
        std::unordered_set<AudioSourceId> activeSounds;
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_AUDIOCOMPONENT_H
