/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SDLEventState.h"

#include <Accela/Platform/SDLUtil.h>

#include <SDL_keyboard.h>

namespace Accela::Platform
{

bool SDLKeyboardState::IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const
{
    const auto scancode = SDLUtil::PhysicalKeyToScanCode(physicalKey);
    if (!scancode)
    {
        return false;
    }

    return IsPhysicalKeyPressed(*scancode);
}

bool SDLKeyboardState::IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const
{
    int numKeys = 0;
    const auto keyState = SDL_GetKeyboardState(&numKeys);

    if (scanCode >= (unsigned int)numKeys)
    {
        return false;
    }

    return keyState[scanCode] == 1;
}

bool SDLKeyboardState::IsModifierPressed(const KeyMod& keyMod) const
{
    switch (keyMod)
    {
        case KeyMod::Control: return IsPhysicalKeyPressed(PhysicalKey::LControl) || IsPhysicalKeyPressed(PhysicalKey::RControl);
        case KeyMod::Shift: return IsPhysicalKeyPressed(PhysicalKey::LShift) || IsPhysicalKeyPressed(PhysicalKey::RShift);
    }

    return false;
}

bool SDLMouseState::IsMouseButtonPressed(const MouseButton& button) const
{
    int x = 0;
    int y = 0;
    const auto buttonState = SDL_GetMouseState(&x, &y);

    switch (button)
    {
        case MouseButton::Left: return buttonState & SDL_BUTTON_LEFT;
        case MouseButton::Middle: return buttonState & SDL_BUTTON_MIDDLE;
        case MouseButton::Right: return buttonState & SDL_BUTTON_RIGHT;
        case MouseButton::X1: return buttonState & SDL_BUTTON_X1;
        case MouseButton::X2: return buttonState & SDL_BUTTON_X2;
    }

    return false;
}

}
