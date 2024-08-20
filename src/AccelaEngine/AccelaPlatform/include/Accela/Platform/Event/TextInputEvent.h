/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_TEXTINPUTEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_TEXTINPUTEVENT_H

#include <Accela/Common/SharedLib.h>

#include <string>

namespace Accela::Platform
{
    /**
     * Represents a keyboard text input event, as reported by the OS
     */
    struct ACCELA_PUBLIC TextInputEvent
    {
        explicit TextInputEvent(std::string _text)
            : text(std::move(_text))
        { }

        std::string text; // UTF8
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_TEXTINPUTEVENT_H
