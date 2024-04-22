/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEBUTTONEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEBUTTONEVENT_H

#include <cstdint>

namespace Accela::Platform
{
    enum class MouseButton
    {
        Left,
        Middle,
        Right,
        X1,
        X2
    };

    enum class ClickType
    {
        Press,
        Release
    };

    /**
     * Represents a mouse button event, as reported by the OS
     */
    struct MouseButtonEvent
    {
        MouseButtonEvent(uint32_t _mouseId, MouseButton _button, ClickType _clickType, uint32_t _clicks, uint32_t _xPos, uint32_t _yPos)
            : mouseId(_mouseId)
            , button(_button)
            , clickType(_clickType)
            , clicks(_clicks)
            , xPos(_xPos)
            , yPos(_yPos)
        { }

        uint32_t mouseId; // Unique id of the mouse, in multi-mouse setups
        MouseButton button; // The mouse button in question
        ClickType clickType; // Whether the mouse button was pressed or released
        uint32_t clicks; // 1 for single-click, 2 for double-click, etc.

        // These positions are specified in window space within the engine, and
        // then *overwritten* with virtual-space positions before being passed to
        // the current Scene as scene events
        uint32_t xPos;
        uint32_t yPos;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEBUTTONEVENT_H
