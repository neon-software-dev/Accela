/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_WINDOWRESIZEEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_WINDOWRESIZEEVENT_H

#include <utility>
#include <cstdint>

namespace Accela::Platform
{
    /**
     * OS event representing the engine's window being resized
     */
    struct WindowResizeEvent
    {
        explicit WindowResizeEvent(std::pair<uint32_t, uint32_t> _size)
            : size(std::move(_size))
        { }

        // In screen-space
        std::pair<uint32_t, uint32_t> size;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_WINDOWRESIZEEVENT_H
