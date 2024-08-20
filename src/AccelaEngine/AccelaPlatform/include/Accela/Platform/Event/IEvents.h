/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IEVENTS_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IEVENTS_H

#include "SystemEvent.h"

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <queue>

namespace Accela::Platform
{
    class IKeyboardState;
    class IMouseState;

    /**
     * Provides access to system/OS events that have occurred
     */
    class ACCELA_PUBLIC IEvents
    {
        public:

            using Ptr = std::shared_ptr<IEvents>;

        public:

            virtual ~IEvents() = default;

            /**
             * Pop the system/OS events that have occurred, local to the Accela window, since the last call to this method.
             *
             * @return The time-sorted queue of system events
             */
            [[nodiscard]] virtual std::queue<SystemEvent> PopLocalEvents() = 0;

            [[nodiscard]] virtual std::shared_ptr<const IKeyboardState> GetKeyboardState() = 0;
            [[nodiscard]] virtual std::shared_ptr<const IMouseState> GetMouseState() = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IEVENTS_H
