/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEMOVEEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEMOVEEVENT_H

#include <Accela/Common/SharedLib.h>

#include <cstdint>

namespace Accela::Platform
{
    /**
     * Represents a mouse movement event, as reported by the OS
     */
    struct ACCELA_PUBLIC MouseMoveEvent
    {
        /**
         * @param pointerId The ID of the pointer/mouse being moved
         * @param xPos The new x position of the mouse, in pixels. (Note that this value is in render space, not window space).
         * @param yPos The new y position of the mouse, in pixels. (Note that this value is in render space, not window space).
         * @param xRel How far the mouse's x position changed, in pixels
         * @param yRel How far the mouse's y position changed, in pixels
         */
        MouseMoveEvent(uint64_t _pointerId, float _xPos, float _yPos, float _xRel, float _yRel)
            : pointerId(_pointerId)
            , xPos(_xPos)
            , yPos(_yPos)
            , xRel(_xRel)
            , yRel(_yRel)
        { }

        uint64_t pointerId; // Unique id of the mouse, in multi-mouse setups

        // These positions are in window space within the engine, and then overwritten with
        // virtual-space positions before being passed to the scene
        float xPos;
        float yPos;

        float xRel;
        float yRel;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEMOVEEVENT_H
