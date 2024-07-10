/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SDLEvents.h"
#include "SDLEventState.h"

#include <Accela/Platform/SDLUtil.h>

namespace Accela::Platform
{

SDLEvents::SDLEvents()
    : m_keyboardState(std::make_shared<SDLKeyboardState>())
    , m_mouseState(std::make_shared<SDLMouseState>())
{

}

std::queue<SystemEvent> SDLEvents::PopLocalEvents()
{
    std::queue<SystemEvent> events{};

    SDL_Event sdlEvent{};
    while (SDL_PollEvent(&sdlEvent))
    {
        std::optional<SystemEvent> systemEvent;

        switch (sdlEvent.type)
        {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                systemEvent = ProcessKeyPressEvent(sdlEvent);
            break;

            case SDL_WINDOWEVENT:
                systemEvent = ProcessWindowEvent(sdlEvent);
            break;

            case SDL_MOUSEMOTION:
                systemEvent = ProcessMouseMoveEvent(sdlEvent);
            break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                systemEvent = ProcessMouseButtonEvent(sdlEvent);
            break;

            case SDL_MOUSEWHEEL:
                systemEvent = ProcessMouseWheelEvent(sdlEvent);
            break;

            case SDL_TEXTINPUT:
                systemEvent = ProcessTextInputEvent(sdlEvent);
            break;
        }

        if (systemEvent)
        {
            events.emplace(systemEvent.value());
        }
    }

    return events;
}

std::shared_ptr<const IKeyboardState> SDLEvents::GetKeyboardState()
{
    return m_keyboardState;
}

std::shared_ptr<const IMouseState> SDLEvents::GetMouseState()
{
    return m_mouseState;
}

std::optional<SystemEvent> SDLEvents::ProcessKeyPressEvent(const SDL_Event& sdlEvent) noexcept
{
    return SDLUtil::SDLKeyEventToKeyEvent(sdlEvent);
}

std::optional<SystemEvent> SDLEvents::ProcessWindowEvent(const SDL_Event& sdlEvent) noexcept
{
    switch (sdlEvent.window.event)
    {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            return WindowResizeEvent({sdlEvent.window.data1, sdlEvent.window.data2});
        case SDL_WINDOWEVENT_CLOSE:
            return WindowCloseEvent{};
    }

    return std::nullopt;
}

std::optional<SystemEvent> SDLEvents::ProcessMouseMoveEvent(const SDL_Event& sdlEvent) noexcept
{
    return MouseMoveEvent(
        sdlEvent.motion.which,
        (float)sdlEvent.motion.x,
        (float)sdlEvent.motion.y,
        (float)sdlEvent.motion.xrel,
        (float)sdlEvent.motion.yrel
    );
}

std::optional<SystemEvent> SDLEvents::ProcessMouseButtonEvent(const SDL_Event& sdlEvent) noexcept
{
    MouseButton button{MouseButton::Left};

    switch (sdlEvent.button.button)
    {
        case SDL_BUTTON_LEFT: button = MouseButton::Left; break;
        case SDL_BUTTON_MIDDLE: button = MouseButton::Middle; break;
        case SDL_BUTTON_RIGHT: button = MouseButton::Right; break;
        case SDL_BUTTON_X1: button = MouseButton::X1; break;
        case SDL_BUTTON_X2: button = MouseButton::X2; break;
    }

    ClickType clickType = ClickType::Press;
    if (sdlEvent.type == SDL_MOUSEBUTTONUP) { clickType = ClickType::Release; }

    return MouseButtonEvent(sdlEvent.button.which, button, clickType, sdlEvent.button.clicks, sdlEvent.button.x, sdlEvent.button.y);
}

std::optional<SystemEvent> SDLEvents::ProcessMouseWheelEvent(const SDL_Event& sdlEvent) noexcept
{
    return MouseWheelEvent(sdlEvent.button.which, sdlEvent.wheel.preciseX, sdlEvent.wheel.preciseY);
}

std::optional<SystemEvent> SDLEvents::ProcessTextInputEvent(const SDL_Event& sdlEvent) noexcept
{
    const auto text = std::string(sdlEvent.text.text);
    if (text.empty())
    {
        return std::nullopt;
    }

    return TextInputEvent(text);
}

}
