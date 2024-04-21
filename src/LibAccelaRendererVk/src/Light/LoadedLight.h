/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H
#define LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H

#include <Accela/Render/Id.h>

#include <Accela/Render/Light.h>

#include <optional>
#include <array>
#include <algorithm>

namespace Accela::Render
{
    struct LoadedLight
    {
        LoadedLight(Light _light, std::optional<FrameBufferId> _framebufferId)
            : light(std::move(_light))
            , shadowFrameBufferId(_framebufferId)
        {
            // If the light has a shadow framebuffer then default its shadow maps to invalidated
            // so that they'll all be rendered/synced
            shadowInvalidated = shadowFrameBufferId.has_value();
        }

        Light light;

        std::optional<FrameBufferId> shadowFrameBufferId;

        bool shadowInvalidated;
    };
}

#endif //LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H
