/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
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

            SDLEvents();

            [[nodiscard]] std::queue<SystemEvent> PopLocalEvents() override;

            [[nodiscard]] std::shared_ptr<const IKeyboardState> GetKeyboardState() override;
            [[nodiscard]] std::shared_ptr<const IMouseState> GetMouseState() override;

        private:

            static std::optional<SystemEvent> ProcessKeyPressEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessWindowEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessMouseMoveEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessMouseButtonEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessMouseWheelEvent(const SDL_Event& sdlEvent) noexcept;
            static std::optional<SystemEvent> ProcessTextInputEvent(const SDL_Event& sdlEvent) noexcept;

        private:

            std::shared_ptr<IKeyboardState> m_keyboardState;
            std::shared_ptr<IMouseState> m_mouseState;
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_EVENTS_SDLEVENTS_H
