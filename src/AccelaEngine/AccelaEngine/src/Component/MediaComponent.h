/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_COMPONENT_MEDIACOMPONENT_H
#define ACCELAENGINE_ACCELAENGINE_SRC_COMPONENT_MEDIACOMPONENT_H

#include "Accela/Engine/Media/MediaCommon.h"

#include <unordered_set>

namespace Accela::Engine
{
    struct MediaComponent
    {
        std::unordered_set<MediaSessionId> activeSessions;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_COMPONENT_MEDIACOMPONENT_H
