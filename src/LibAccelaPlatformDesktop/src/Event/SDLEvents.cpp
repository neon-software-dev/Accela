/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SDLEvents.h"

namespace Accela::Platform
{

std::queue<SystemEvent> SDLEvents::PopSystemEvents()
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
        }

        if (systemEvent)
        {
            events.emplace(systemEvent.value());
        }
    }

    return events;
}

std::optional<SystemEvent> SDLEvents::ProcessKeyPressEvent(const SDL_Event& sdlEvent) noexcept
{
    const auto keyAction = sdlEvent.type == SDL_KEYDOWN ? KeyEvent::Action::KeyPress : KeyEvent::Action::KeyRelease;
    const auto key = SDLKeysymToKey(sdlEvent.key.keysym);

    return KeyEvent(keyAction, key);
}

Key SDLEvents::SDLKeysymToKey(const SDL_Keysym& keysym) noexcept
{
    switch (keysym.sym)
    {
        case SDLK_ESCAPE: return Key::Escape;
        case SDLK_LCTRL: return Key::LeftControl;
        case SDLK_BACKSPACE: return Key::Backspace;
        case SDLK_KP_ENTER: return Key::Keypad_Enter;
        case SDLK_RETURN: return Key::Return;

        case SDLK_a: return Key::A;
        case SDLK_b: return Key::B;
        case SDLK_c: return Key::C;
        case SDLK_d: return Key::D;
        case SDLK_e: return Key::E;
        case SDLK_f: return Key::F;
        case SDLK_g: return Key::G;
        case SDLK_h: return Key::H;
        case SDLK_i: return Key::I;
        case SDLK_j: return Key::J;
        case SDLK_k: return Key::K;
        case SDLK_l: return Key::L;
        case SDLK_m: return Key::M;
        case SDLK_n: return Key::N;
        case SDLK_o: return Key::O;
        case SDLK_p: return Key::P;
        case SDLK_q: return Key::Q;
        case SDLK_r: return Key::R;
        case SDLK_s: return Key::S;
        case SDLK_t: return Key::T;
        case SDLK_u: return Key::U;
        case SDLK_v: return Key::V;
        case SDLK_w: return Key::W;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;
        case SDLK_0: return Key::Zero;
        case SDLK_1: return Key::One;
        case SDLK_2: return Key::Two;
        case SDLK_3: return Key::Three;
        case SDLK_4: return Key::Four;
        case SDLK_5: return Key::Five;
        case SDLK_6: return Key::Six;
        case SDLK_7: return Key::Seven;
        case SDLK_8: return Key::Eight;
        case SDLK_9: return Key::Nine;
        case SDLK_SPACE: return Key::Space;
        case SDLK_PERIOD: return Key::Period;
        case SDLK_QUESTION: return Key::Question;
        case SDLK_COMMA: return Key::Comma;
        case SDLK_BACKQUOTE: return Key::BackQuote;
        case SDLK_MINUS: return keysym.mod & KMOD_LSHIFT ? Key::Underscore :Key::Minus;

        default: return Key::Unknown;
    }
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
        sdlEvent.motion.x,
        sdlEvent.motion.y,
        sdlEvent.motion.xrel,
        sdlEvent.motion.yrel
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

}
