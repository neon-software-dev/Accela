/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IEVENTS_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IEVENTS_H

#include "SystemEvent.h"

#include <memory>
#include <queue>

namespace Accela::Platform
{
    /**
     * Provides access to system/OS events that have occurred
     */
    class IEvents
    {
        public:

            using Ptr = std::shared_ptr<IEvents>;

        public:

            virtual ~IEvents() = default;

            /**
             * Pop the system/OS events that have occurred since the last call to this method.
             *
             * @return The time-sorted queue of system events
             */
            [[nodiscard]] virtual std::queue<SystemEvent> PopSystemEvents() = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IEVENTS_H
