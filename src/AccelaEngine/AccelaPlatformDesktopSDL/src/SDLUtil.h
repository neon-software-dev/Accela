/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_SDLUTIL_H
#define LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_SDLUTIL_H

#include <Accela/Platform/Event/KeyEvent.h>

#include <SDL2/SDL_events.h>

#include <optional>

namespace Accela::Platform
{
    class SDLUtil
    {
        public:

            [[nodiscard]] static std::optional<KeyEvent> SDLKeyEventToKeyEvent(const SDL_Event& event);

            [[nodiscard]] static std::optional<ScanCode> PhysicalKeyToScanCode(const PhysicalKey& physicalKey);
    };
}

#endif //LIBACCELAPLATFORMDESKTOP_INCLUDE_ACCELA_PLATFORM_SDLUTIL_H
