/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_AUDIO_AUDIOSOURCEPROPERTIES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_AUDIO_AUDIOSOURCEPROPERTIES_H

namespace Accela::Engine
{
    /**
     * Defines properties that can be applied when playing an audio source
     */
    struct AudioSourceProperties
    {
        // Whether the audio loops (repeats)
        bool looping{false};

        // The distance in world space that no attenuation occurs. At 0.0, no distance
        // attenuation ever occurs on non-linear attenuation models.
        float referenceDistance{1.0f};

        // A value of 1.0 means unattenuated. Each division by 2 equals an attenuation of
        // about -6dB. Each multiplicaton by 2 equals an amplification of about +6dB. A value
        // of 0.0 is meaningless with respect to a logarithmic scale; it is silent.
        float gain{1.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_AUDIO_AUDIOSOURCEPROPERTIES_H
