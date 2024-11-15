/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_SYSTEMEVENT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_SYSTEMEVENT_H

#include "KeyEvent.h"
#include "WindowResizeEvent.h"
#include "WindowCloseEvent.h"
#include "MouseMoveEvent.h"
#include "MouseButtonEvent.h"
#include "MouseWheelEvent.h"
#include "TextInputEvent.h"

#include <variant>

namespace Accela::Platform
{
    // Variant defining all possible system events
    using SystemEvent = std::variant<
        KeyEvent,
        WindowResizeEvent,
        WindowCloseEvent,
        MouseMoveEvent,
        MouseButtonEvent,
        MouseWheelEvent,
        TextInputEvent
    >;
}


#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_SYSTEMEVENT_H
