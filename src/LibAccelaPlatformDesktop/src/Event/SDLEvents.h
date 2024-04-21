/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_EVENTS_SDLEVENTS_H
#define LIBACCELAPLATFORMSDL_SRC_EVENTS_SDLEVENTS_H

#include "Accela/Platform/Event/IEvents.h"

#include <SDL2/SDL_events.h>

#include <optional>

namespace Accela::Platform
{
    class SDLEvents : public IEvents
    {
        public:

            [[nodiscard]] std::queue<SystemEvent> PopSystemEvents() override;

        private:

            [[nodiscard]] static Key SDLKeysymToKey(const SDL_Keysym& keysym) noexcept;

            static std::optional<SystemEvent> ProcessKeyPressEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessWindowEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessMouseMoveEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessMouseButtonEvent(const SDL_Event& sdlEvent) noexcept;
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_EVENTS_SDLEVENTS_H
