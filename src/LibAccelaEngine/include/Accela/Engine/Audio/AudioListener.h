/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_AUDIO_AUDIOLISTENER_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_AUDIO_AUDIOLISTENER_H

#include <glm/glm.hpp>

namespace Accela::Engine
{
    /**
     * Defines the properties relevant to an audio listener: the listener's position and orientation
     */
    struct AudioListener
    {
        AudioListener() = default;

        AudioListener(const glm::vec3& _worldPosition, const glm::vec3& _lookUnit, const glm::vec3& _upUnit)
            : worldPosition(_worldPosition)
            , lookUnit(_lookUnit)
            , upUnit(_upUnit)
        { }

        // The listener's position, in world space
        glm::vec3 worldPosition{0.0f, 0.0f, 0.0f};

        // The listener's orientation unit vector
        glm::vec3 lookUnit{0.0f, 0.0f, -1.0f};

        // Up-vector for the listener's orientation
        glm::vec3 upUnit{0.0f, 1.0f, 0.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_AUDIO_AUDIOLISTENER_H
