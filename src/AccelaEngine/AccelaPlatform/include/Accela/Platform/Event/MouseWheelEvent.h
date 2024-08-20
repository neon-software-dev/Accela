/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEWHEELEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEWHEELEVENT_H

#include <Accela/Common/SharedLib.h>

#include <cstdint>

namespace Accela::Platform
{
    /**
     * Represents a mouse wheel scroll, as reported by the OS.
     *
     * Warning! The scroll amounts have different meanings in different platform
     * subsystems. Using Qt the scrolls are reported in eights of a degree whereas
     * in SDL the scrolls seem to be reported simply as +1 and -1 for direction of
     * travel.
     */
    struct ACCELA_PUBLIC MouseWheelEvent
    {
        MouseWheelEvent(uint32_t _mouseId, float _scrollX, float _scrollY)
            : mouseId(_mouseId)
            , scrollX(_scrollX)
            , scrollY(_scrollY)
        { }

        uint32_t mouseId; // Unique id of the mouse, in multi-mouse setups
        float scrollX; // Amount scrolled horizontally
        float scrollY; // Amount scrolled vertically
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEWHEELEVENT_H
