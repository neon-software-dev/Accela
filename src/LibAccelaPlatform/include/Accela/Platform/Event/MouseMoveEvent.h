/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEMOVEEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEMOVEEVENT_H

#include <cstdint>

namespace Accela::Platform
{
    /**
     * Represents a mouse movement event, as reported by the OS
     */
    struct MouseMoveEvent
    {
        /**
         * @param pointerId The ID of the pointer/mouse being moved
         * @param xPos The new x position of the mouse, in pixels. (Note that this value is in render space, not window space).
         * @param yPos The new y position of the mouse, in pixels. (Note that this value is in render space, not window space).
         * @param xRel How far the mouse's x position changed, in pixels
         * @param yRel How far the mouse's y position changed, in pixels
         */
        MouseMoveEvent(uint32_t _pointerId, unsigned int _xPos, unsigned int _yPos, int _xRel, int _yRel)
            : pointerId(_pointerId)
            , xPos(_xPos)
            , yPos(_yPos)
            , xRel(_xRel)
            , yRel(_yRel)
        { }

        uint32_t pointerId; // Unique id of the mouse, in multi-mouse setups

        // These positions are in window space within the engine, and then overwritten with
        // virtual-space positions before being passed to the scene
        unsigned int xPos;
        unsigned int yPos;

        int xRel;
        int yRel;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSEMOVEEVENT_H
