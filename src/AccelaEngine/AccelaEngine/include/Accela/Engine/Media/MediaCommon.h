/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_INCLUDE_ACCELA_ENGINE_MEDIA_MEDIACOMMON_H
#define ACCELAENGINE_ACCELAENGINE_INCLUDE_ACCELA_ENGINE_MEDIA_MEDIACOMMON_H

#include <Accela/Common/Id.h>

#include <chrono>

namespace Accela::Engine
{
    DEFINE_ID_TYPE(MediaSessionId)

    // A duration of time
    using MediaDuration = std::chrono::duration<float>; // Seconds

    // A particular point of time within a media stream
    using MediaPoint = MediaDuration;
}

DEFINE_ID_HASH(Accela::Engine::MediaSessionId)

#endif //ACCELAENGINE_ACCELAENGINE_INCLUDE_ACCELA_ENGINE_MEDIA_MEDIACOMMON_H
